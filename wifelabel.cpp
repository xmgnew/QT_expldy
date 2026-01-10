#include "wifelabel.h"

#include <QDir>
#include <QFileInfoList>
#include <QPainter>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>

#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QSlider>
#include <QSettings>
#include <QRandomGenerator>

#include "itemwidget.h"

WifeLabel::WifeLabel(QWidget *parent)
    : QLabel(parent)
{
    connect(&frameTimer, &QTimer::timeout, this, [this]()
            {
        if (currentFrames.isEmpty()) return;
        frameIndex = (frameIndex + 1) % currentFrames.size();
        setPixmap(currentFrames[frameIndex]); });

    edgeHitCooldown.start();
    loadUserSettings();

    // Happy/Angry 这类“短情绪态”共用一个计时器
    happyTimer.setSingleShot(true);
    connect(&happyTimer, &QTimer::timeout, this, [this]()
            {
                // 只有仍然处于短情绪态才回 idle（避免被别的状态覆盖）
                if (mainState == State::Happy || mainState == State::Angry)
                {
                    mainState = State::Idle;
                    playMainState();
                }
            });
}

void WifeLabel::loadUserSettings()
{
    // 组织名/应用名随你改；先写死一个稳定值即可
    QSettings s("expldy", "expldy");
    volume = s.value("audio/volume", 70).toInt();
    frequency = s.value("audio/frequency", 50).toInt();

    // 防御一下范围
    volume = std::clamp(volume, 0, 100);
    frequency = std::clamp(frequency, 0, 100);
}

void WifeLabel::saveUserSettings() const
{
    QSettings s("expldy", "expldy");
    s.setValue("audio/volume", volume);
    s.setValue("audio/frequency", frequency);
}

void WifeLabel::setTargetSize(QSize s)
{
    targetSize = s;
}

QString WifeLabel::assetsRoot() const
{
    const QString cwd = QDir::currentPath();
    const QString appDir = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        QDir(cwd).filePath("assets"),
        QDir(appDir).filePath("assets"),
        QDir(appDir).filePath("../assets"),
        QDir(appDir).filePath("../../assets"),
    };

    for (const auto &p : candidates)
    {
        if (QDir(p).exists())
            return p;
    }
    return {};
}

QPixmap WifeLabel::normalizeFrame(const QPixmap &src, const QSize &targetSize)
{
    if (src.isNull())
        return QPixmap();

    QPixmap scaled = src.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap canvas(targetSize);
    canvas.fill(Qt::transparent);

    QPainter p(&canvas);
    int x = (targetSize.width() - scaled.width()) / 2;
    int y = (targetSize.height() - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);

    return canvas;
}

QVector<QPixmap> WifeLabel::loadFrames(const QString &dirPath) const
{
    QVector<QPixmap> frames;
    QDir dir(dirPath);
    if (!dir.exists())
        return frames;

    QFileInfoList files = dir.entryInfoList({"*.png", "*.PNG"}, QDir::Files, QDir::Name);
    for (const auto &fi : files)
    {
        QPixmap raw(fi.absoluteFilePath());
        QPixmap norm = normalizeFrame(raw, targetSize);
        if (!norm.isNull())
            frames.push_back(norm);
    }
    return frames;
}

bool WifeLabel::loadFromAssets()
{
    const QString root = assetsRoot();
    if (root.isEmpty())
    {
        setText("Cannot find assets/ folder");
        adjustSize();
        return false;
    }

    const QString base = QDir(root).filePath("wife");
    idleFrames = loadFrames(QDir(base).filePath("idle"));
    happyFrames = loadFrames(QDir(base).filePath("happy"));
    angryFrames = loadFrames(QDir(base).filePath("angry"));
    hitFrames = loadFrames(QDir(base).filePath("hit"));
    draggingFrames = loadFrames(QDir(base).filePath("dragging"));

    qDebug() << "assetsRoot =" << root
             << "idle=" << idleFrames.size()
             << "happy=" << happyFrames.size()
             << "angry=" << angryFrames.size()
             << "hit=" << hitFrames.size()
             << "dragging=" << draggingFrames.size();

    if (idleFrames.isEmpty())
    {
        setText("No idle frames in assets/wife/idle");
        adjustSize();
        return false;
    }

    if (draggingFrames.isEmpty())
        draggingFrames = idleFrames;

    frameIndex = 0;
    setPixmap(idleFrames[0]);
    resize(idleFrames[0].size());

    // --- 阶段1：初始化音频与物品库 ---
    audio.setAssetsRoot(root);
    audio.rebuildIndex();
    audio.setVolume01(volume / 100.0);

    // 物品帧尺寸：先统一 64x64（后续可做成设置）
    itemDB.load(root, QSize(64, 64));

    return true;
}

void WifeLabel::spawnItem(const QString& itemId)
{
    QWidget* w = window();
    if (!w) return;

    const ItemDef* def = itemDB.get(itemId);
    if (!def) return;

    auto* item = new ItemWidget(def->id, def->frames, def->frameIntervalMs, w);

    // 默认生成在角色旁边（右下角一点）
    QPoint p = this->mapTo(w, QPoint(width() - 20, height() - 20));
    item->move(p);

    item->show();
    item->raise();

    // 可选：spawn 音效（不影响阶段2“使用食物”测试）
    if (def->audio.contains("item_spawn"))
        audio.playSfx(def->audio.value("item_spawn"));
    if (def->audio.contains("enemy_spawn"))
        audio.playEnemy(def->audio.value("enemy_spawn"));
    if (def->audio.contains("actor_spawn"))
        audio.playVoice(def->audio.value("actor_spawn"));
}

void WifeLabel::setFrames(const QVector<QPixmap> &frames, int intervalMs)
{
    frameTimer.stop();

    currentFrames = frames;
    frameIndex = 0;

    if (currentFrames.isEmpty())
        return;

    setPixmap(currentFrames[0]);

    if (currentFrames.size() > 1)
        frameTimer.start(intervalMs);
}

void WifeLabel::playMainState()
{
    switch (mainState)
    {
    case State::Idle:
        setFrames(idleFrames, 80);
        break;
    case State::Happy:
        setFrames(happyFrames.isEmpty() ? idleFrames : happyFrames, 80);
        break;
    case State::Angry:
        setFrames(angryFrames.isEmpty() ? idleFrames : angryFrames, 80);
        break;
    case State::Dragging:
        setFrames(draggingFrames.isEmpty() ? idleFrames : draggingFrames, 80);
        break;
    }
}

void WifeLabel::playIdle()
{
    mainState = State::Idle;
    playMainState();
}

void WifeLabel::playHappy()
{
    audio.playRandom("happy");
    mainState = State::Happy;
    playMainState();

    // 随机 happy 时长：800–1400 ms
    int durationMs = QRandomGenerator::global()->bounded(800, 1401);
    happyTimer.start(durationMs);
}

void WifeLabel::playAngry()
{
    audio.playRandom("angry");
    mainState = State::Angry;
    playMainState();

    // 随机 angry 时长：800–1400 ms
    int durationMs = QRandomGenerator::global()->bounded(800, 1401);
    happyTimer.start(durationMs);
}



void WifeLabel::playHit(int ms)
{
    audio.playRandom("hit");
    if (hitFrames.isEmpty())
        return;

    setFrames(hitFrames, 60);

    QTimer::singleShot(ms, this, [this]()
                       { playMainState(); });
}

void WifeLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        pressedLeft = true;
        dragging = false;

        pressGlobalPos = event->globalPosition().toPoint();
        labelStartPos = pos();

        playHit(200); // 轻触反馈
    }

    QLabel::mousePressEvent(event);
}

void WifeLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!pressedLeft)
        return;
    if (!(event->buttons() & Qt::LeftButton))
        return;

    QPoint nowGlobal = event->globalPosition().toPoint();
    QPoint delta = nowGlobal - pressGlobalPos;

    if (!dragging)
    {
        if (delta.manhattanLength() < dragThresholdPx)
            return;
        dragging = true;

        mainState = State::Dragging;
        audio.playRandom("dragging");
        playMainState();
        happyTimer.stop(); 
    }

    QPoint newPos = labelStartPos + delta;

    QWidget *p = parentWidget();
    if (!p)
    {
        move(newPos);
        return;
    }

    int minX = 0, minY = 0;
    int maxX = p->width() - width();
    int maxY = p->height() - height();
    if (maxX < minX)
        maxX = minX;
    if (maxY < minY)
        maxY = minY;

    bool hitEdge =
        (newPos.x() <= minX) || (newPos.x() >= maxX) ||
        (newPos.y() <= minY) || (newPos.y() >= maxY);

    newPos.setX(std::clamp(newPos.x(), minX, maxX));
    newPos.setY(std::clamp(newPos.y(), minY, maxY));

    move(newPos);

    const int cooldownMs = 1000;
    if (hitEdge && edgeHitCooldown.elapsed() > cooldownMs)
    {
        edgeHitCooldown.restart();
        playHit(300);
    }
}

void WifeLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        pressedLeft = false;
        dragging = false;
        happyTimer.stop();
        mainState = State::Idle;
        playMainState();
    }

    QLabel::mouseReleaseEvent(event);
}

void WifeLabel::contextMenuEvent(QContextMenuEvent *event)
{
    // 右键优先：强制结束拖动/回 idle（你选的 A）
    pressedLeft = false;
    dragging = false;
    mainState = State::Idle;
    playMainState();
    happyTimer.stop();

    QMenu menu(this);

    // Inventory
    QAction* inv = menu.addAction("Inventory...");
    connect(inv, &QAction::triggered, this, [this]() {
        if (!inventoryDlg) {
            inventoryDlg = new InventoryDialog(window());
            inventoryDlg->setDB(&itemDB);
            connect(inventoryDlg, &InventoryDialog::spawnRequested, this, [this](const QString& id) {
                spawnItem(id);
            });
        } else {
            // 如果物品库已刷新（未来热重载），这里可以重建 UI
            inventoryDlg->setDB(&itemDB);
        }

        inventoryDlg->show();
        inventoryDlg->raise();
        inventoryDlg->activateWindow();
    });

    // --- Interact 子菜单 ---
    QMenu *interact = menu.addMenu("Interact");
    QAction *patHead = interact->addAction("Pat head");
    QAction *spank = interact->addAction("Spank");
    QAction *feed = interact->addAction("Feed");

    connect(patHead, &QAction::triggered, this, [this]()
            {
        // 先简单点：摸头 -> happy 一下
        playHappy(); });

    connect(spank, &QAction::triggered, this, [this]()
            {
        // 打屁股 -> angry（短情绪态）
        playAngry(); });

    connect(feed, &QAction::triggered, this, [this]()
            {
        // 喂食 -> happy
        playHappy(); });

    menu.addSeparator();

    // --- Audio 子菜单：Volume / Frequency sliders ---
    QMenu *audioMenu = menu.addMenu("Audio");

    // Volume slider
    {
        QWidgetAction *wa = new QWidgetAction(audioMenu);
        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(volume);
        slider->setMinimumWidth(160);
        wa->setDefaultWidget(slider);
        audioMenu->addAction(wa);

        // 立即生效一次
        audio.setVolume01(volume / 100.0);

        connect(slider, &QSlider::valueChanged, this, [this](int v)
                {
                    volume = v;
                    saveUserSettings();
                    audio.setVolume01(volume / 100.0);
                });
    }

    // Frequency slider
    {
        QWidgetAction *wa = new QWidgetAction(audioMenu);
        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setValue(frequency);
        slider->setMinimumWidth(160);
        wa->setDefaultWidget(slider);
        audioMenu->addAction(wa);

        connect(slider, &QSlider::valueChanged, this, [this](int v)
                {
                    frequency = v;
                    saveUserSettings();
                    // 里程碑5：这里会更新“随机语音间隔策略”
                });
    }

    menu.addSeparator();

    // Quit
    QAction *quit = menu.addAction("Quit");
    connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);

    menu.exec(event->globalPos());
    event->accept();
}
