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
#include <QHash>
#include "audiomanager.h"

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

    // Idle clip 系统：idle/ 下每个子目录是一个 clip
    QHash<QString, QVector<QPixmap>> idleClips;
    QString currentIdleClip;
    QString lastIdleClip;
    QVector<QPixmap> idleFrames; // currentIdleClip 对应的帧缓存（兼容旧逻辑）

    QVector<QPixmap> happyFrames;
    QVector<QPixmap> angryFrames;
    QVector<QPixmap> hitFrames;
    QVector<QPixmap> draggingFrames;

    QVector<QPixmap> currentFrames;
    int frameIndex = 0;

    // Timer
    QTimer frameTimer;
    QTimer emotionTimer;     // Happy/Angry 用同一个 timer
    QTimer idleSwitchTimer;  // Idle clip 切换

    int idleSwitchIntervalMs() const; // 由 frequency 决定
    void startOrStopIdleSwitchTimer();
    void switchIdleClipRandom();

    QString assetsRoot() const;
    QVector<QPixmap> loadFrames(const QString &dirPath) const;
    static QPixmap normalizeFrame(const QPixmap &src, const QSize &targetSize);

    void setFrames(const QVector<QPixmap> &frames, int intervalMs);
    void playMainState();

    // 拖动（窗口内拖动 label）
    bool pressedLeft = false;
    QPoint dragOffset;
    bool dragging = false;
    QPoint pressGlobalPos;
    QPoint labelStartPos;
    int dragThresholdPx = 8;

    // 撞边 hit 冷却
    QElapsedTimer edgeHitCooldown;

    // 右键菜单设置（先存值，里程碑5再接音频）
    int volume = 70;       // 0-100
    int frequency = 50;    // 0-100（说话频率/健谈程度）
    AudioManager audio;

    void loadUserSettings();
    void saveUserSettings() const;
};
