#include "audiomanager.h"

#include <QDir>
#include <QFileInfoList>
#include <QRandomGenerator>
#include <QUrl>
#include <algorithm>

AudioManager::AudioManager(QObject* parent) : QObject(parent)
{
    voicePlayer.setAudioOutput(&voiceOut);
    sfxPlayer.setAudioOutput(&sfxOut);
    enemyPlayer.setAudioOutput(&enemyOut);

    setVolume01(volume01);
}

void AudioManager::setVolume01(double v01)
{
    volume01 = std::clamp(v01, 0.0, 1.0);

    // UI 0.0-1.0 映射到 0.0-2.0 的软件增益（和你之前一致）
    const double boosted = std::clamp(volume01 * 2.0, 0.0, 2.0);
    voiceOut.setVolume(float(boosted));
    sfxOut.setVolume(float(boosted));
    enemyOut.setVolume(float(boosted));
}

void AudioManager::setAssetsRoot(const QString& assetsRoot)
{
    root = assetsRoot;
}

QStringList AudioManager::scanAudioFiles(const QString& dirPath) const
{
    QStringList out;
    QDir dir(dirPath);
    if (!dir.exists()) return out;

    const QFileInfoList files = dir.entryInfoList(
        {"*.wav","*.WAV","*.ogg","*.OGG","*.mp3","*.MP3"},
        QDir::Files, QDir::Name);

    for (const auto& fi : files)
        out << fi.absoluteFilePath();

    return out;
}

void AudioManager::rebuildBankFromDir(QHash<QString, QStringList>& outBank, const QString& baseDir)
{
    outBank.clear();
    QDir base(baseDir);
    if (!base.exists()) return;

    // 每个子目录就是一个 category
    const QFileInfoList dirs = base.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto& d : dirs) {
        const QString category = d.fileName();
        const QStringList files = scanAudioFiles(d.absoluteFilePath());
        if (!files.isEmpty())
            outBank.insert(category, files);
    }
}

void AudioManager::rebuildIndex()
{
    voiceBank.clear();
    sfxBank.clear();
    enemyBank.clear();
    if (root.isEmpty()) return;

    const QString audioBase = QDir(root).filePath("audio");
    rebuildBankFromDir(voiceBank, QDir(audioBase).filePath("wife"));
    rebuildBankFromDir(sfxBank, QDir(audioBase).filePath("items"));
    rebuildBankFromDir(enemyBank, QDir(audioBase).filePath("monsters"));
}

void AudioManager::stop()
{
    voicePlayer.stop();
    sfxPlayer.stop();
    enemyPlayer.stop();
}

void AudioManager::playFromBank(QMediaPlayer& p, const QHash<QString, QStringList>& bank, const QString& category)
{
    const auto list = bank.value(category);
    if (list.isEmpty()) return;

    const int idx = QRandomGenerator::global()->bounded(list.size());
    const QString path = list.at(idx);

    p.stop();
    p.setSource(QUrl::fromLocalFile(path));
    p.play();
}

void AudioManager::playVoice(const QString& category)
{
    playFromBank(voicePlayer, voiceBank, category);
}

void AudioManager::playSfx(const QString& category)
{
    playFromBank(sfxPlayer, sfxBank, category);
}

void AudioManager::playEnemy(const QString& category)
{
    playFromBank(enemyPlayer, enemyBank, category);
}

void AudioManager::playRandom(const QString& category)
{
    // 兼容旧调用：默认走 Voice
    playVoice(category);
}
