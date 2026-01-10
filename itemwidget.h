#pragma once
#include <QLabel>
#include <QString>
#include <QMouseEvent>

class ItemWidget : public QLabel {
public:
    explicit ItemWidget(const QString& itemId, QWidget* parent = nullptr);

    QString itemId() const { return id; }

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    QString id;
    bool pressed = false;
    QPoint pressGlobal;
    QPoint startPos;
    int dragThreshold = 4;
};
