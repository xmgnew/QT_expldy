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

class WifeLabel : public QLabel
{
public:
    explicit WifeLabel(QWidget *parent = nullptr);

    void setTargetSize(QSize s);
    bool loadFromAssets(); // 从 assets/wife/... 加载帧

    void playIdle();
    void playHappy();
    void playHit(int ms = 200); // 短反馈：播 hit 后回到主状态

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    enum class State { Idle, Happy, Dragging };
    State mainState = State::Idle; // 主状态（不会被 hit 短反馈打断）

    QSize targetSize { 400, 600 };

    QVector<QPixmap> idleFrames;
    QVector<QPixmap> happyFrames;
    QVector<QPixmap> hitFrames;
    QVector<QPixmap> draggingFrames;

    QVector<QPixmap> currentFrames;
    int frameIndex = 0;
    QTimer frameTimer;

    QString assetsRoot() const; // 自动定位 assets 目录
    QVector<QPixmap> loadFrames(const QString &dirPath) const;
    static QPixmap normalizeFrame(const QPixmap &src, const QSize &targetSize);

    void setFrames(const QVector<QPixmap> &frames, int intervalMs);
    void playMainState();

    // 拖动（窗口内拖动 label）
    bool pressedLeft = false;
    bool dragging = false;
    QPoint pressPosInLabel;
    QPoint pressGlobalPos;   // 新增
    QPoint labelStartPos;
    int dragThresholdPx = 8;



    // 撞边 hit 冷却（如果你还在用）
    QElapsedTimer edgeHitCooldown;
};
