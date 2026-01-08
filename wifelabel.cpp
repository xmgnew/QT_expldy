#include "wifelabel.h"

#include <QDir>
#include <QFileInfoList>
#include <QPainter>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>

WifeLabel::WifeLabel(QWidget *parent)
    : QLabel(parent)
{
    connect(&frameTimer, &QTimer::timeout, this, [this]() {
        if (currentFrames.isEmpty()) return;
        frameIndex = (frameIndex + 1) % currentFrames.size();
        setPixmap(currentFrames[frameIndex]);
    });
}

void WifeLabel::setTargetSize(QSize s)
{
    targetSize = s;
}

QString WifeLabel::assetsRoot() const
{
    const QString cwd = QDir::currentPath();
    const QString appDir = QCoreApplication::applicationDirPath();

    const QStringList candidates = {
        QDir(cwd).filePath("assets"),
        QDir(appDir).filePath("assets"),
        QDir(appDir).filePath("../assets"),
        QDir(appDir).filePath("../../assets"),
    };

    for (const auto &p : candidates) {
        if (QDir(p).exists())
            return p;
    }
    return {};
}

QPixmap WifeLabel::normalizeFrame(const QPixmap &src, const QSize &targetSize)
{
    if (src.isNull())
        return QPixmap();

    QPixmap scaled = src.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap canvas(targetSize);
    canvas.fill(Qt::transparent);

    QPainter p(&canvas);
    int x = (targetSize.width() - scaled.width()) / 2;
    int y = (targetSize.height() - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);

    return canvas;
}

QVector<QPixmap> WifeLabel::loadFrames(const QString &dirPath) const
{
    QVector<QPixmap> frames;
    QDir dir(dirPath);
    if (!dir.exists())
        return frames;

    QFileInfoList files = dir.entryInfoList({"*.png", "*.PNG"}, QDir::Files, QDir::Name);
    for (const auto &fi : files) {
        QPixmap raw(fi.absoluteFilePath());
        QPixmap norm = normalizeFrame(raw, targetSize);
        if (!norm.isNull())
            frames.push_back(norm);
    }
    return frames;
}

bool WifeLabel::loadFromAssets()
{
    const QString root = assetsRoot();
    if (root.isEmpty()) {
        setText("Cannot find assets/ folder");
        adjustSize();
        return false;
    }

    const QString base = QDir(root).filePath("wife");
    idleFrames     = loadFrames(QDir(base).filePath("idle"));
    happyFrames    = loadFrames(QDir(base).filePath("happy"));
    hitFrames      = loadFrames(QDir(base).filePath("hit"));
    draggingFrames = loadFrames(QDir(base).filePath("dragging"));

    qDebug() << "assetsRoot =" << root
             << "idle=" << idleFrames.size()
             << "happy=" << happyFrames.size()
             << "hit=" << hitFrames.size()
             << "dragging=" << draggingFrames.size();

    if (idleFrames.isEmpty()) {
        setText("No idle frames in assets/wife/idle");
        adjustSize();
        return false;
    }

    // dragging 允许没有，fallback 到 idle
    if (draggingFrames.isEmpty())
        draggingFrames = idleFrames;

    // 初始显示第一帧
    frameIndex = 0;
    setPixmap(idleFrames[0]);
    resize(idleFrames[0].size());
    return true;
}

void WifeLabel::setFrames(const QVector<QPixmap> &frames, int intervalMs)
{
    frameTimer.stop();

    currentFrames = frames;
    frameIndex = 0;

    if (currentFrames.isEmpty())
        return;

    setPixmap(currentFrames[0]);

    if (currentFrames.size() > 1)
        frameTimer.start(intervalMs);
}

void WifeLabel::playMainState()
{
    switch (mainState) {
    case State::Idle:
        setFrames(idleFrames, 80);
        break;
    case State::Happy:
        setFrames(happyFrames.isEmpty() ? idleFrames : happyFrames, 80);
        break;
    case State::Dragging:
        setFrames(draggingFrames.isEmpty() ? idleFrames : draggingFrames, 80);
        break;
    }
}

void WifeLabel::playIdle()
{
    mainState = State::Idle;
    playMainState();
}

void WifeLabel::playHappy()
{
    mainState = State::Happy;
    playMainState();
}

void WifeLabel::playHit(int ms)
{
    if (hitFrames.isEmpty())
        return;

    // 先播放 hit（短反馈）
    setFrames(hitFrames, 60);

    QTimer::singleShot(ms, this, [this]() {
        playMainState(); // 回到主状态（Idle / Dragging / Happy）
    });
}

void WifeLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressedLeft = true;
        dragging = false;

        pressPosInLabel = event->pos();
        pressGlobalPos = event->globalPosition().toPoint();
        labelStartPos = pos();

        // 轻触反馈（A）
        playHit(100);
    }

    QLabel::mousePressEvent(event);
}

void WifeLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!pressedLeft) return;
    if (!(event->buttons() & Qt::LeftButton)) return;

    QPoint nowGlobal = event->globalPosition().toPoint();
    QPoint delta = nowGlobal - pressGlobalPos;


    if (!dragging) {
        if (delta.manhattanLength() < dragThresholdPx) return;
        dragging = true;

        mainState = State::Dragging;
        playMainState();
    }

    QPoint newPos = labelStartPos + delta;

    // 限制不出窗口（避免拖没）
    QWidget *p = parentWidget();
    if (p) {
        int minX = 0, minY = 0;
        int maxX = p->width() - width();
        int maxY = p->height() - height();
        if (maxX < minX) maxX = minX;
        if (maxY < minY) maxY = minY;

        newPos.setX(std::clamp(newPos.x(), minX, maxX));
        newPos.setY(std::clamp(newPos.y(), minY, maxY));
    }

    move(newPos);
}

void WifeLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        pressedLeft = false;
        dragging = false;

        mainState = State::Idle;
        playMainState();
    }

    QLabel::mouseReleaseEvent(event);
}

void WifeLabel::contextMenuEvent(QContextMenuEvent *event)
{
    // 右键优先：强制结束拖动/回 idle（里程碑3再弹菜单）
    pressedLeft = false;
    dragging = false;

    mainState = State::Idle;
    playMainState();

    event->accept();
}
