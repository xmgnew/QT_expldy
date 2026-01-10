#include "animateditembutton.h"
#include <QIcon>

AnimatedItemButton::AnimatedItemButton(const QString &itemId,
                                       const QVector<QPixmap> &f,
                                       int intervalMs,
                                       QWidget *parent)
    : QPushButton(parent), id(itemId), frames(f)
{
    setMinimumSize(72, 72);
    setText("");
    setCheckable(false);

    refreshIcon();

    if (frames.size() > 1)
    {
        timer.start(intervalMs);
        connect(&timer, &QTimer::timeout, this, [this]()
                {
            idx = (idx + 1) % frames.size();
            refreshIcon(); });
    }
}

void AnimatedItemButton::refreshIcon()
{
    if (frames.isEmpty())
        return;
    setIcon(QIcon(frames[idx]));
    setIconSize(QSize(56, 56));
}
