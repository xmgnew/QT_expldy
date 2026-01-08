#pragma once
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QPixmap>
#include <QSize>

class WifeLabel : public QLabel
{
public:
    explicit WifeLabel(QWidget *parent = nullptr);

    void setTargetSize(QSize s);
    bool loadFromAssets();                 // 从 assets/wife/... 加载帧

    void playIdle();
    void playHappy();
    void playHit(int ms = 200);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    enum class State { Idle, Happy, Hit };

    QSize targetSize { 400, 600 };
    State state = State::Idle;

    QVector<QPixmap> idleFrames;
    QVector<QPixmap> happyFrames;
    QVector<QPixmap> hitFrames;

    int frameIndex = 0;
    QTimer frameTimer;

    QString assetsRoot() const;            // 自动定位 assets 目录
    QVector<QPixmap> loadFrames(const QString& dirPath) const;
    static QPixmap normalizeFrame(const QPixmap& src, const QSize& targetSize);

    void setFrames(const QVector<QPixmap>& frames, int intervalMs);
};
