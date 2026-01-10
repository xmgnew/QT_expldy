#pragma once

#include <QLabel>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QPixmap>
#include <QSize>
#include <QPoint>
#include <QMouseEvent>
#include <QContextMenuEvent>

#include "audiomanager.h"
#include "itemdb.h"
#include "inventorydialog.h"

class WifeLabel : public QLabel
{
public:
    explicit WifeLabel(QWidget *parent = nullptr);

    void setTargetSize(QSize s);
    bool loadFromAssets(); // 从 assets/wife/... 加载帧

    void playIdle();
    void playHappy();
    void playAngry();
    void playHit(int ms = 200); // 短反馈：播 hit 后回到主状态

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    enum class State { Idle, Happy, Angry, Dragging };
    State mainState = State::Idle;

    QSize targetSize { 400, 600 };

    QVector<QPixmap> idleFrames;
    QVector<QPixmap> happyFrames;
    QVector<QPixmap> angryFrames;
    QVector<QPixmap> hitFrames;
    QVector<QPixmap> draggingFrames;

    QVector<QPixmap> currentFrames;
    int frameIndex = 0;

    // Timer
    QTimer frameTimer;
    QTimer happyTimer;

    // 阶段1：音频（三通道）+ 物品库 + 物品栏
    AudioManager audio;
    ItemDB itemDB;
    InventoryDialog* inventoryDlg = nullptr;

    void spawnItem(const QString& itemId);

    QString assetsRoot() const;
    QVector<QPixmap> loadFrames(const QString &dirPath) const;
    static QPixmap normalizeFrame(const QPixmap &src, const QSize &targetSize);

    void setFrames(const QVector<QPixmap> &frames, int intervalMs);
    void playMainState();

    // 拖动（窗口内拖动 label）
    bool pressedLeft = false;
    bool dragging = false;
    QPoint pressGlobalPos;
    QPoint labelStartPos;
    int dragThresholdPx = 8;

    // 撞边 hit 冷却
    QElapsedTimer edgeHitCooldown;

    // 右键菜单设置（先存值，里程碑5再接音频）
    int volume = 70;       // 0-100
    int frequency = 50;    // 0-100（说话频率/健谈程度）

    void loadUserSettings();
    void saveUserSettings() const;
};
