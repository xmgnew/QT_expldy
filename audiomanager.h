#pragma once
#include <QObject>
#include <QHash>
#include <QStringList>

#include <QMediaPlayer>
#include <QAudioOutput>

// AudioManager
// - Voice: 角色发声（assets/audio/wife/<category>/）
// - SFX:   物品音效（assets/audio/items/<category>/）
// - Enemy: 怪物音效（assets/audio/monsters/<category>/）
// 三个通道彼此独立，可同时播放。

class AudioManager : public QObject
{
    Q_OBJECT
public:
    explicit AudioManager(QObject *parent = nullptr);

    void setVolume01(double v); // 0.0-1.0
    void setAssetsRoot(const QString &assetsRoot);
    void rebuildIndex();

    // Backward-compat: 等同于 playVoice(category)
    void playRandom(const QString &category);

    void stop();

    // 三通道播放
    void playVoice(const QString &category);
    void playSfx(const QString &category);
    void playEnemy(const QString &category);

private:
    QString root;
    double volume01 = 0.7;

    QMediaPlayer voicePlayer;
    QAudioOutput voiceOut;

    QMediaPlayer sfxPlayer;
    QAudioOutput sfxOut;

    QMediaPlayer enemyPlayer;
    QAudioOutput enemyOut;

    QHash<QString, QStringList> voiceBank;
    QHash<QString, QStringList> sfxBank;
    QHash<QString, QStringList> enemyBank;

    QStringList scanAudioFiles(const QString &dirPath) const;

    void rebuildBankFromDir(QHash<QString, QStringList> &outBank, const QString &baseDir);
    void playFromBank(QMediaPlayer &p, const QHash<QString, QStringList> &bank, const QString &category);
};
