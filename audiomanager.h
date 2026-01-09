#pragma once
#include <QObject>
#include <QHash>
#include <QStringList>

#include <QMediaPlayer>
#include <QAudioOutput>

class AudioManager : public QObject {
    Q_OBJECT
public:
    explicit AudioManager(QObject* parent=nullptr);

    void setVolume01(double v);               // 0.0-1.0
    void setAssetsRoot(const QString& assetsRoot);
    void rebuildIndex();

    void stop();
    void playRandom(const QString& category);

private:
    QString root;
    double volume01 = 0.7;

    QMediaPlayer player;
    QAudioOutput output;

    QHash<QString, QStringList> bank;
    QStringList scanAudioFiles(const QString& dirPath) const;
};
