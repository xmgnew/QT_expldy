#include "wifelabel.h"
#include <QDir>
#include <QFileInfoList>
#include <QPainter>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QDebug>

WifeLabel::WifeLabel(QWidget *parent)
    : QLabel(parent)
{
    connect(&frameTimer, &QTimer::timeout, this, [this]() {
        const QVector<QPixmap>* frames = nullptr;
        if (state == State::Idle)  frames = &idleFrames;
        if (state == State::Happy) frames = &happyFrames;
        if (state == State::Hit)   frames = &hitFrames;

        if (!frames || frames->isEmpty()) return;

        frameIndex = (frameIndex + 1) % frames->size();
        setPixmap((*frames)[frameIndex]);
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

    for (const auto& p : candidates) {
        if (QDir(p).exists()) return p;
    }
    return {};
}

QPixmap WifeLabel::normalizeFrame(const QPixmap& src, const QSize& targetSize)
{
    if (src.isNull()) return QPixmap();

    QPixmap scaled = src.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QPixmap canvas(targetSize);
    canvas.fill(Qt::transparent);

    QPainter p(&canvas);
    int x = (targetSize.width()  - scaled.width())  / 2;
    int y = (targetSize.height() - scaled.height()) / 2;
    p.drawPixmap(x, y, scaled);

    return canvas;
}

QVector<QPixmap> WifeLabel::loadFrames(const QString& dirPath) const
{
    QVector<QPixmap> frames;
    QDir dir(dirPath);
    if (!dir.exists()) return frames;

    QFileInfoList files = dir.entryInfoList({ "*.png", "*.PNG" }, QDir::Files, QDir::Name);
    for (const auto& fi : files) {
        QPixmap raw(fi.absoluteFilePath());
        QPixmap norm = normalizeFrame(raw, targetSize);
        if (!norm.isNull()) frames.push_back(norm);
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
    idleFrames  = loadFrames(QDir(base).filePath("idle"));
    happyFrames = loadFrames(QDir(base).filePath("happy"));
    hitFrames   = loadFrames(QDir(base).filePath("hit"));

    qDebug() << "assetsRoot =" << root
             << "idle=" << idleFrames.size()
             << "happy=" << happyFrames.size()
             << "hit=" << hitFrames.size();

    if (idleFrames.isEmpty()) {
        setText("No idle frames in assets/wife/idle");
        adjustSize();
        return false;
    }

    frameIndex = 0;
    setPixmap(idleFrames[0]);
    resize(idleFrames[0].size());
    return true;
}

void WifeLabel::setFrames(const QVector<QPixmap>& frames, int intervalMs)
{
    if (frames.isEmpty()) return;

    frameTimer.stop();
    frameIndex = 0;

    setPixmap(frames[0]);
    resize(frames[0].size());

    if (frames.size() > 1) frameTimer.start(intervalMs);
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

    QTimer::singleShot(ms, this, [this]() {
        playIdle();
    });
}

void WifeLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        playHit(200);
    }
    QLabel::mousePressEvent(event);
}
