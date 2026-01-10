#pragma once
#include <QPushButton>
#include <QVector>
#include <QPixmap>
#include <QTimer>

class AnimatedItemButton : public QPushButton
{
public:
    AnimatedItemButton(const QString &itemId,
                       const QVector<QPixmap> &frames,
                       int intervalMs,
                       QWidget *parent = nullptr);

    QString itemId() const { return id; }

private:
    QString id;
    QVector<QPixmap> frames;
    int idx = 0;
    QTimer timer;

    void refreshIcon();
};
