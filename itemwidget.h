#pragma once
#include <QLabel>
#include <QString>
#include <QMouseEvent>
#include <QTimer>
#include <QVector>
#include <QPixmap>

class ItemWidget : public QLabel {
    Q_OBJECT
public:
    explicit ItemWidget(const QString& itemId,
                        const QVector<QPixmap>& frames,
                        int intervalMs,
                        QWidget* parent = nullptr);

    QString itemId() const { return id; }

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

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
};
