#include "wifelabel.h"
#include <QDir>
#include <QFileInfoList>
#include <QPainter>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>

WifeLabel::WifeLabel(QWidget *parent)
    : QLabel(parent)
{
    connect(&frameTimer, &QTimer::timeout, this, [this]()
            {
        const QVector<QPixmap>* frames = nullptr;
        if (state == State::Idle)  frames = &idleFrames;
        if (state == State::Happy) frames = &happyFrames;
        if (state == State::Hit)   frames = &hitFrames;

        if (!frames || frames->isEmpty()) return;

        frameIndex = (frameIndex + 1) % frames->size();
        setPixmap((*frames)[frameIndex]); });
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

    for (const auto &p : candidates)
    {
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
    for (const auto &fi : files)
    {
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
    if (root.isEmpty())
    {
        setText("Cannot find assets/ folder");
        adjustSize();
        return false;
    }

    const QString base = QDir(root).filePath("wife");
    idleFrames = loadFrames(QDir(base).filePath("idle"));
    happyFrames = loadFrames(QDir(base).filePath("happy"));
    hitFrames = loadFrames(QDir(base).filePath("hit"));

    qDebug() << "assetsRoot =" << root
             << "idle=" << idleFrames.size()
             << "happy=" << happyFrames.size()
             << "hit=" << hitFrames.size();

    if (idleFrames.isEmpty())
    {
        setText("No idle frames in assets/wife/idle");
        adjustSize();
        return false;
    }

    frameIndex = 0;
    setPixmap(idleFrames[0]);
    resize(idleFrames[0].size());
    return true;
}

void WifeLabel::setFrames(const QVector<QPixmap> &frames, int intervalMs)
{
    if (frames.isEmpty())
        return;

    frameTimer.stop();
    frameIndex = 0;

    setPixmap(frames[0]);
    resize(frames[0].size());

    if (frames.size() > 1)
        frameTimer.start(intervalMs);
}

void WifeLabel::playIdle()
{
    state = State::Idle;
    setFrames(idleFrames, 80);
}

void WifeLabel::playHappy()
{
    state = State::Happy;
    setFrames(happyFrames.isEmpty() ? idleFrames : happyFrames, 80);
}

void WifeLabel::playHit(int ms)
{
    state = State::Hit;
    setFrames(hitFrames.isEmpty() ? idleFrames : hitFrames, 60);

    QTimer::singleShot(ms, this, [this]()
                       { playIdle(); });
}

void WifeLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragging = true;
        dragOffset = event->pos(); // 鼠标在控件内部的位置
        edgeHitCooldown.start();   // 开始计时
        playHit(200);              // 点击反馈：hit -> 回 idle
    }
    QLabel::mousePressEvent(event);
}

void WifeLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (!dragging)
        return;

    QPoint parentPos = mapToParent(event->pos());
    QPoint newPos = parentPos - dragOffset;

    QWidget *p = parentWidget();
    if (!p)
    {
        move(newPos);
        return;
    }

    int minX = 0;
    int minY = 0;
    int maxX = p->width() - width();
    int maxY = p->height() - height();

    maxX = std::max(maxX, minX);
    maxY = std::max(maxY, minY);

    bool outX = (newPos.x() < minX) || (newPos.x() > maxX);
    bool outY = (newPos.y() < minY) || (newPos.y() > maxY);
    bool outOfBounds = outX || outY;

    int clampedX = std::clamp(newPos.x(), minX, maxX);
    int clampedY = std::clamp(newPos.y(), minY, maxY);

    // 如果试图拖出边界：触发 hit（加冷却，避免疯狂闪）
    if (outOfBounds)
    {
        // 250ms 内只触发一次
        if (!edgeHitCooldown.isValid() || edgeHitCooldown.elapsed() > 250)
        {
            edgeHitCooldown.restart();
            playHit(200);
        }
    }

    // 位置永远夹紧在窗口内（无法继续拖出）
    move(clampedX, clampedY);
}

void WifeLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragging = false;
    }
    QLabel::mouseReleaseEvent(event);
}
