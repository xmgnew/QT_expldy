#include "itemwidget.h"
#include <QApplication>

ItemWidget::ItemWidget(const QString& itemId, QWidget* parent)
    : QLabel(parent), id(itemId)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
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
    }
    QLabel::mouseReleaseEvent(e);
}
