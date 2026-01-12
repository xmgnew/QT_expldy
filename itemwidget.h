#pragma once
#include <QLabel>
#include <QString>
#include <QMouseEvent>
#include <QTimer>
#include <QVector>
#include <QPixmap>

class ItemWidget : public QLabel
{
    Q_OBJECT
public:
    bool isEquipped() const { return equipped; }
    void setEquipped(bool v) { equipped = v; }

    // 用于怪物等“生成到场景里”的物体标记（阶段1/2：最小闭环）
    bool isSpawned() const { return spawned; }
    void setSpawned(bool v) { spawned = v; }
    explicit ItemWidget(const QString &itemId,
                        const QVector<QPixmap> &frames,
                        int intervalMs,
                        QWidget *parent = nullptr);

    QString itemId() const { return id; }

signals:
    void dropped(ItemWidget *item);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QString id;
    QVector<QPixmap> frames;
    int idx = 0;
    QTimer anim;

    bool pressed = false;
    QPoint pressGlobal;
    QPoint startPos;
    int dragThreshold = 4;

    void refreshFrame();
    bool equipped = false;
    bool spawned = false;
};
