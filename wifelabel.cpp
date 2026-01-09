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
#include <QHBoxLayout>
#include <QLabel>

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

    emotionTimer.setSingleShot(true);
    connect(&emotionTimer, &QTimer::timeout, this, [this]()
            {
                // 只有仍然处于短情绪态才回 idle（避免被别的状态覆盖）
                if (mainState == State::Happy || mainState == State::Angry)
                {
                    mainState = State::Idle;
                    playMainState();
                } });

    connect(&idleSwitchTimer, &QTimer::timeout, this, [this]()
            {
                // 只在 idle 时允许切换（否则会打断交互状态）
                if (mainState == State::Idle)
                    switchIdleClipRandom(); });
}

int WifeLabel::idleSwitchIntervalMs() const
{
    // frequency: 0-100
    // 0 代表完全不切换；1-100 映射到一个“从慢到快”的切换间隔
    if (frequency <= 0)
        return -1;

    // 线性映射：1 -> 20000ms，100 -> 2000ms
    const int f = std::clamp(frequency, 1, 100);
    const int slowMs = 20000;
    const int fastMs = 2000;
    const double t = (f - 1) / 99.0;
    const int ms = int(slowMs + (fastMs - slowMs) * t);
    return std::clamp(ms, fastMs, slowMs);
}

void WifeLabel::startOrStopIdleSwitchTimer()
{
    const int ms = idleSwitchIntervalMs();
    if (ms < 0)
    {
        idleSwitchTimer.stop();
        return;
    }

    // 重新启动（立即按新的频率生效）
    idleSwitchTimer.start(ms);
}

void WifeLabel::switchIdleClipRandom()
{
    if (idleClips.isEmpty())
        return;
    const QString beforeClip = currentIdleClip;
    if (idleClips.size() == 1)
    {
        auto keys = idleClips.keys();
        std::sort(keys.begin(), keys.end());
        currentIdleClip = keys.first();
    }

    else
    {
        QString chosen;
        // 尽量避免连续重复：最多尝试 10 次（够用且不浪费）
        for (int i = 0; i < 10; ++i)
        {
            const int idx = QRandomGenerator::global()->bounded(idleClips.size());
            const auto keys = idleClips.keys();
            const QString key = keys.at(idx);

            if (key != currentIdleClip)
            {
                chosen = key;
                break;
            }
        }
        if (chosen.isEmpty())
        {
            // 实在没选到（理论上不会），就强制选一个不同的
            const auto keys = idleClips.keys();
            chosen = keys.at((keys.indexOf(currentIdleClip) + 1) % keys.size());
        }
        lastIdleClip = currentIdleClip;
        currentIdleClip = chosen;
    }

    // 如果没有发生切换，就不播 idle 音频、不刷新
    if (!beforeClip.isEmpty() && currentIdleClip == beforeClip)
        return;

    idleFrames = idleClips.value(currentIdleClip);
    if (idleFrames.isEmpty())
        return;

    // 只有 idle 时才立刻切换画面；否则只更新缓存，等回 idle 再用
    if (mainState == State::Idle)
    {
        audio.playRandom("idle"); // 只在切换 idle clip 时播一次
        playMainState();
    }
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

    // --- Idle clips ---
    idleClips.clear();
    currentIdleClip.clear();
    lastIdleClip.clear();
    {
        QDir idleDir(QDir(base).filePath("idle"));
        if (idleDir.exists())
        {
            const QFileInfoList clipDirs = idleDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
            for (const auto &fi : clipDirs)
            {
                const QString clipName = fi.fileName();
                const auto frames = loadFrames(fi.absoluteFilePath());
                if (!frames.isEmpty())
                    idleClips.insert(clipName, frames);
            }

            // 兼容旧结构：如果 idle/ 下直接放了 png，也当成一个 clip(\"default\")
            const auto directFrames = loadFrames(idleDir.absolutePath());
            if (!directFrames.isEmpty() && !idleClips.contains("default"))
                idleClips.insert("default", directFrames);
        }
    }

    if (!idleClips.isEmpty())
    {
        auto keys = idleClips.keys();
        std::sort(keys.begin(), keys.end()); //  稳定：按字母序
        currentIdleClip = keys.first();
        idleFrames = idleClips.value(currentIdleClip);
    }

    happyFrames = loadFrames(QDir(base).filePath("happy"));
    angryFrames = loadFrames(QDir(base).filePath("angry"));
    hitFrames = loadFrames(QDir(base).filePath("hit"));
    draggingFrames = loadFrames(QDir(base).filePath("dragging"));

    qDebug() << "assetsRoot =" << root
             << "idleClips=" << idleClips.size()
             << "idleFrames=" << idleFrames.size() << "(clip" << currentIdleClip << ")"
             << "happy=" << happyFrames.size()
             << "angry=" << angryFrames.size()
             << "hit=" << hitFrames.size()
             << "dragging=" << draggingFrames.size();

    if (idleFrames.isEmpty())
    {
        setText("No idle clips in assets/wife/idle/<clip>/000.png");
        adjustSize();
        return false;
    }

    if (draggingFrames.isEmpty())
        draggingFrames = idleFrames;

    frameIndex = 0;
    setPixmap(idleFrames[0]);
    resize(idleFrames[0].size());

    startOrStopIdleSwitchTimer();

    audio.setAssetsRoot(root); // root = assetsRoot()
    audio.rebuildIndex();
    audio.setVolume01(volume / 100.0);

    return true;
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
    emotionTimer.start(durationMs);
}

void WifeLabel::playAngry()
{
    audio.playRandom("angry");
    mainState = State::Angry;
    playMainState();

    // 随机 angry 时长：800–1400 ms
    int durationMs = QRandomGenerator::global()->bounded(800, 1401);
    emotionTimer.start(durationMs);
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
        emotionTimer.stop();
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

    const int cooldownMs = 500;
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
        emotionTimer.stop();
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
    emotionTimer.stop();

    QMenu menu(this);

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

    // 一个小工具：创建“标题 + slider + 数字”的行
    auto addLabeledSlider = [this, audioMenu](
                                const QString &title,
                                int minV, int maxV, int value,
                                std::function<void(int)> onChanged)
    {
        QWidget *row = new QWidget(audioMenu);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(8, 6, 8, 6);
        layout->setSpacing(8);

        QLabel *name = new QLabel(title, row);
        name->setMinimumWidth(72);

        QSlider *slider = new QSlider(Qt::Horizontal, row);
        slider->setRange(minV, maxV);
        slider->setValue(value);
        slider->setMinimumWidth(160);

        QLabel *num = new QLabel(QString::number(value), row);
        num->setMinimumWidth(32);
        num->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        layout->addWidget(name);
        layout->addWidget(slider, 1);
        layout->addWidget(num);

        QWidgetAction *wa = new QWidgetAction(audioMenu);
        wa->setDefaultWidget(row);
        audioMenu->addAction(wa);

        // 联动：显示数字 + 业务逻辑
        connect(slider, &QSlider::valueChanged, row, [num, onChanged](int v)
                {
        num->setText(QString::number(v));
        onChanged(v); });

        return slider;
    };

    // Volume（0-100）
    addLabeledSlider("Volume", 0, 100, volume, [this](int v)
                     {
    volume = v;
    saveUserSettings();
    audio.setVolume01(volume / 100.0); });

    // Frequency（0-100）
    addLabeledSlider("Frequency", 0, 100, frequency, [this](int v)
                     {
    frequency = v;
    saveUserSettings();
    startOrStopIdleSwitchTimer(); });

    menu.addSeparator();

    // Quit
    QAction *quit = menu.addAction("Quit");
    connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);

    menu.exec(event->globalPos());
    event->accept();
}
