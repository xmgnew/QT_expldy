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
                if (mainState == State::Happy || mainState == State::Angry || mainState == State::Eat ||
                    mainState == State::Attack || mainState == State::Defend)
                {
                    mainState = State::Idle;
                    playMainState();
                } });

    // idle clip 随机切换：只在 Idle 时触发
    connect(&idleSwitchTimer, &QTimer::timeout, this, [this]()
            {
        if (mainState == State::Idle)
            switchIdleClipRandom(true); });
}

int WifeLabel::idleSwitchIntervalMs() const
{
    // frequency: 0-100
    // 0 代表完全不切换；1-100 映射到一个“从慢到快”的切换间隔
    if (frequency <= 0)
        return -1;

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
    idleSwitchTimer.start(ms);
}

void WifeLabel::switchIdleClipRandom(bool playVoice)
{
    if (idleClips.isEmpty())
        return;

    // 只有一个 clip：固定
    if (idleClips.size() == 1)
    {
        auto keys = idleClips.keys();
        std::sort(keys.begin(), keys.end());
        currentIdleClip = keys.first();
    }
    else
    {
        QString chosen;
        const auto keys = idleClips.keys();
        // 尽量避免连续重复：最多尝试 10 次
        for (int i = 0; i < 10; ++i)
        {
            const int idx = QRandomGenerator::global()->bounded(keys.size());
            const QString k = keys.at(idx);
            if (k != currentIdleClip)
            {
                chosen = k;
                break;
            }
        }
        if (chosen.isEmpty())
        {
            // 兜底：选一个不同的
            const int cur = keys.indexOf(currentIdleClip);
            chosen = keys.at((cur + 1 + keys.size()) % keys.size());
        }
        lastIdleClip = currentIdleClip;
        currentIdleClip = chosen;
    }

    const auto frames = idleClips.value(currentIdleClip);
    if (frames.isEmpty())
        return;

    idleFrames = frames;

    // 切换 clip 时播放 idle 语音（只在 idle 态生效）
    if (playVoice && mainState == State::Idle)
        audio.playVoice("idle");

    if (mainState == State::Idle)
        playMainState();
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

            // 兼容：如果 idle/ 下直接放了 png，也当成一个 clip("default")
            const auto directFrames = loadFrames(idleDir.absolutePath());
            if (!directFrames.isEmpty() && !idleClips.contains("default"))
                idleClips.insert("default", directFrames);
        }
    }

    if (!idleClips.isEmpty())
    {
        auto keys = idleClips.keys();
        std::sort(keys.begin(), keys.end());
        currentIdleClip = keys.first();
        idleFrames = idleClips.value(currentIdleClip);
    }

    happyFrames = loadFrames(QDir(base).filePath("happy"));
    angryFrames = loadFrames(QDir(base).filePath("angry"));
    eatFrames = loadFrames(QDir(base).filePath("eat"));
    attackFrames = loadFrames(QDir(base).filePath("attack"));
    defendFrames = loadFrames(QDir(base).filePath("defend"));
    hitFrames = loadFrames(QDir(base).filePath("hit"));
    draggingFrames = loadFrames(QDir(base).filePath("dragging"));

    qDebug() << "assetsRoot =" << root
             << "idleClips=" << idleClips.size()
             << "idleFrames=" << idleFrames.size() << "(clip" << currentIdleClip << ")"
             << "happy=" << happyFrames.size()
             << "angry=" << angryFrames.size()
             << "eat=" << eatFrames.size()
             << "attack=" << attackFrames.size()
             << "defend=" << defendFrames.size()
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

    // --- 阶段1：初始化音频与物品库 ---
    audio.setAssetsRoot(root);
    audio.rebuildIndex();
    audio.setVolume01(volume / 100.0);

    // 物品帧尺寸：先统一 64x64（后续可做成设置）
    itemDB.load(root, QSize(64, 64));

    // 让 idle 随机切换策略立即生效
    startOrStopIdleSwitchTimer();

    return true;
}

void WifeLabel::spawnItem(const QString &itemId)
{
    QWidget *w = window();
    if (!w)
        return;

    const ItemDef *def = itemDB.get(itemId);
    if (!def)
        return;

    auto *item = new ItemWidget(def->id, def->frames, def->frameIntervalMs, w);

    // 默认生成在角色旁边（右下角一点）
    QPoint p = this->mapTo(w, QPoint(width() - 20, height() - 20));
    item->move(p);

    item->show();
    item->raise();

    // 阶段2：拖拽物品松手时，判定是否“使用在角色身上”
    connect(item, &ItemWidget::dropped, this, [this](ItemWidget *it)
            { handleItemDropped(it); });

    // 可选：spawn 音效（不影响阶段2“使用食物”测试）
    if (def->audio.contains("item_spawn"))
        audio.playSfx(def->audio.value("item_spawn"));
    if (def->audio.contains("enemy_spawn"))
        audio.playEnemy(def->audio.value("enemy_spawn"));
    if (def->audio.contains("actor_spawn"))
        audio.playVoice(def->audio.value("actor_spawn"));
}

void WifeLabel::handleItemDropped(ItemWidget *item)
{

    if (!item)
        return;

    const ItemDef *def = itemDB.get(item->itemId());
    if (!def)
        return;

    const bool onChar = overlapsCharacter(item);

    switch (def->type)
    {
    case ItemType::Food:
        if (onChar)
        {
            // 角色+物品双音效（如果 manifest 配了）
            if (def->audio.contains("actor_use"))
                audio.playVoice(def->audio.value("actor_use"));
            else
                audio.playVoice("eat"); // 没配就默认

            if (def->audio.contains("item_use"))
                audio.playSfx(def->audio.value("item_use"));

            playEat();           // 已经加了 assets/wife/eat 的话就播 eat
            item->deleteLater(); // 食物消失
        }
        break;

    case ItemType::Weapon:
        if (onChar)
        {
            equip(item, ItemType::Weapon); // 装备不消失
            // 你想的话，这里也可以触发一次 attack 音效/动画作为反馈
            if (def->audio.contains("actor_use"))
                audio.playVoice(def->audio.value("actor_use"));
            if (def->audio.contains("item_use"))
                audio.playSfx(def->audio.value("item_use"));

            // ✅ 动画反馈：优先 attack（没有 attack 帧就自动用 happy 顶替）
            playAttack();
        }
        else
        {
            // 只有“曾经装备过”的武器，拖离角色才销毁
            if (item->isEquipped())
            {
                if (equippedWeapon == item)
                    equippedWeapon = nullptr;
                item->deleteLater();
            }
        }
        break;

    case ItemType::Shield:
        if (onChar)
        {
            equip(item, ItemType::Shield);
            if (def->audio.contains("actor_use"))
                audio.playVoice(def->audio.value("actor_use"));
            if (def->audio.contains("item_use"))
                audio.playSfx(def->audio.value("item_use"));

            // ✅ 动画反馈：defend（没有 defend 帧就自动用 idle 顶替）
            playDefend();
        }
        else
        {
            if (item->isEquipped())
            {
                if (equippedShield == item)
                    equippedShield = nullptr;
                item->deleteLater();
            }
        }
        break;

    case ItemType::Monster:
        if (onChar)
        {
            // 阶段2.5：先做最小 spawn（不 eat）
            if (def->audio.contains("enemy_spawn"))
                audio.playEnemy(def->audio.value("enemy_spawn"));
            // 先不销毁，先让怪物留在场景里
        }
        break;

    case ItemType::Misc:
    default:
        // 先不处理
        break;
    }
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
    case State::Eat:
        setFrames(eatFrames.isEmpty() ? (happyFrames.isEmpty() ? idleFrames : happyFrames) : eatFrames, 80);
        break;
    case State::Attack:
        // attack 动画不存在就先用 happy 顶替（后续你补 assets/wife/attack 即可）
        setFrames(attackFrames.isEmpty() ? (happyFrames.isEmpty() ? idleFrames : happyFrames) : attackFrames, 80);
        break;
    case State::Defend:
        // defend 动画不存在就先用 idle 顶替
        setFrames(defendFrames.isEmpty() ? idleFrames : defendFrames, 80);
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

void WifeLabel::playEat()
{
    // 注意：食物交互里会根据 manifest 播 actor_use，这里不强制播音，避免重复。
    mainState = State::Eat;
    playMainState();

    int durationMs = QRandomGenerator::global()->bounded(800, 1401);
    happyTimer.start(durationMs);
}

void WifeLabel::playAttack()
{
    // 注意：物品交互里会根据 manifest 播 actor_use / item_use，这里不强制播音，避免重复。
    mainState = State::Attack;
    playMainState();

    int durationMs = QRandomGenerator::global()->bounded(800, 1401);
    happyTimer.start(durationMs);
}

void WifeLabel::playDefend()
{
    mainState = State::Defend;
    playMainState();

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
        snapEquippedItems();
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
    snapEquippedItems();

    const int cooldownMs = 600;
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
    QAction *inv = menu.addAction("Inventory...");
    connect(inv, &QAction::triggered, this, [this]()
            {
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
        inventoryDlg->activateWindow(); });

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
                    audio.setVolume01(volume / 100.0); });
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
                    // frequency 控制 idle clip 随机切换间隔
                    startOrStopIdleSwitchTimer(); });
    }

    menu.addSeparator();

    // Quit
    QAction *quit = menu.addAction("Quit");
    connect(quit, &QAction::triggered, qApp, &QCoreApplication::quit);

    menu.exec(event->globalPos());
    event->accept();
}

bool WifeLabel::overlapsCharacter(QWidget *item) const
{
    QWidget *w = window();
    if (!w || !item)
        return false;

    QRect charRect(mapTo(w, QPoint(0, 0)), size());
    QRect itemRect(item->mapTo(w, QPoint(0, 0)), item->size());
    return charRect.intersects(itemRect);
}

void WifeLabel::snapEquippedItems()
{
    QWidget *w = window();
    if (!w)
        return;

    const QPoint charTopLeft = mapTo(w, QPoint(0, 0));

    // 没有显式设置挂点时，用“相对角色尺寸”的默认挂点。
    // 这样 targetSize / 角色尺寸变化时，武器/盾不会飞到右下角很远。
    const QPoint weaponAnchor = (weaponOffset.x() < 0 || weaponOffset.y() < 0)
                                   ? QPoint(int(width() * 0.72), int(height() * 0.60))
                                   : weaponOffset;
    const QPoint shieldAnchor = (shieldOffset.x() < 0 || shieldOffset.y() < 0)
                                   ? QPoint(int(width() * 0.30), int(height() * 0.62))
                                   : shieldOffset;

    if (equippedWeapon)
    {
        equippedWeapon->move(charTopLeft + weaponAnchor - QPoint(equippedWeapon->width() / 2, equippedWeapon->height() / 2));
        equippedWeapon->raise();
    }
    if (equippedShield)
    {
        equippedShield->move(charTopLeft + shieldAnchor - QPoint(equippedShield->width() / 2, equippedShield->height() / 2));
        equippedShield->raise();
    }
}

void WifeLabel::equip(ItemWidget *item, ItemType type)
{
    if (!item)
        return;

    item->setEquipped(true);

    if (type == ItemType::Weapon)
    {
        if (equippedWeapon && equippedWeapon != item)
            equippedWeapon->deleteLater();
        equippedWeapon = item;
    }
    else if (type == ItemType::Shield)
    {
        if (equippedShield && equippedShield != item)
            equippedShield->deleteLater();
        equippedShield = item;
    }

    snapEquippedItems();
}
