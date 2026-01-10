#include "itemwidget.h"

ItemWidget::ItemWidget(const QString& itemId,
                       const QVector<QPixmap>& f,
                       int intervalMs,
                       QWidget* parent)
    : QLabel(parent), id(itemId), frames(f)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setScaledContents(false);

    refreshFrame();

    if (frames.size() > 1) {
        anim.start(intervalMs);
        connect(&anim, &QTimer::timeout, this, [this]() {
            idx = (idx + 1) % frames.size();
            refreshFrame();
        });
    }
}

void ItemWidget::refreshFrame()
{
    if (frames.isEmpty()) return;
    setPixmap(frames[idx]);
    resize(frames[idx].size());
}

void ItemWidget::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        pressed = true;
        pressGlobal = e->globalPosition().toPoint();
        startPos = pos();
        raise();
    }
    QLabel::mousePressEvent(e);
}

void ItemWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!pressed) return;
    if (!(e->buttons() & Qt::LeftButton)) return;

    QPoint now = e->globalPosition().toPoint();
    QPoint delta = now - pressGlobal;
    if (delta.manhattanLength() < dragThreshold) return;

    move(startPos + delta);
}

void ItemWidget::mouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        pressed = false;
        emit dropped(this);
    }
    QLabel::mouseReleaseEvent(e);
}
