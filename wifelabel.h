#pragma once

#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QPixmap>
#include <QSize>
#include <QPoint>
#include <QHash>
#include <QString>
#include <QMouseEvent>
#include <QContextMenuEvent>

#include "audiomanager.h"
#include "itemdb.h"
#include "inventorydialog.h"

class ItemWidget;

class WifeLabel : public QLabel
{
public:
    explicit WifeLabel(QWidget *parent = nullptr);

    void setTargetSize(QSize s);
    bool loadFromAssets(); // 从 assets/wife/... 加载帧

    void playIdle();
    void playHappy();
    void playAngry();
    void playEat();
    void playAttack();
    void playDefend();
    void playHit(int ms = 200); // 短反馈：播 hit 后回到主状态

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    enum class State
    {
        Idle,
        Happy,
        Angry,
        Eat,
        Attack,
        Defend,
        Dragging
    };
    State mainState = State::Idle;

    QSize targetSize{400, 600};

    QVector<QPixmap> idleFrames;
    // Idle clip 系统：idle/ 下每个子文件夹 = 一个 clip
    QHash<QString, QVector<QPixmap>> idleClips;
    QString currentIdleClip;
    QString lastIdleClip;

    QVector<QPixmap> happyFrames;
    QVector<QPixmap> angryFrames;
    QVector<QPixmap> eatFrames;
    QVector<QPixmap> attackFrames;
    QVector<QPixmap> defendFrames;
    QVector<QPixmap> hitFrames;
    QVector<QPixmap> draggingFrames;

    QVector<QPixmap> currentFrames;
    int frameIndex = 0;

    // Timer
    QTimer frameTimer;
    QTimer happyTimer;
    QTimer idleSwitchTimer;

    // 阶段1：音频（三通道）+ 物品库 + 物品栏
    AudioManager audio;
    ItemDB itemDB;
    InventoryDialog *inventoryDlg = nullptr;

    void spawnItem(const QString &itemId);
    void handleItemDropped(ItemWidget *item);

    QString assetsRoot() const;
    QVector<QPixmap> loadFrames(const QString &dirPath) const;
    static QPixmap normalizeFrame(const QPixmap &src, const QSize &targetSize);

    void setFrames(const QVector<QPixmap> &frames, int intervalMs);
    void playMainState();

    int idleSwitchIntervalMs() const;
    void startOrStopIdleSwitchTimer();
    void switchIdleClipRandom(bool playVoice = true);

    // 拖动（窗口内拖动 label）
    bool pressedLeft = false;
    bool dragging = false;
    QPoint pressGlobalPos;
    QPoint labelStartPos;
    int dragThresholdPx = 8;

    // 撞边 hit 冷却
    QElapsedTimer edgeHitCooldown;

    // 右键菜单设置（先存值，里程碑5再接音频）
    int volume = 70;    // 0-100
    int frequency = 50; // 0-100（说话频率/健谈程度）

    void loadUserSettings();
    void saveUserSettings() const;

    ItemWidget *equippedWeapon = nullptr;
    ItemWidget *equippedShield = nullptr;

    // 装备挂点（角色本地坐标）。如果为 (-1,-1) 则用基于当前角色尺寸的默认值。
    // 这样你换 targetSize/素材尺寸也不会跑偏。
    QPoint weaponOffset = QPoint(-1, -1);
    QPoint shieldOffset = QPoint(-1, -1);

    void snapEquippedItems(); // 角色移动时让装备跟随
    bool overlapsCharacter(QWidget *item) const;
    void equip(ItemWidget *item, ItemType type);
};
