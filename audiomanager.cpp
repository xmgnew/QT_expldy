#include "audiomanager.h"
#include <QDir>
#include <QFileInfoList>
#include <QUrl>
#include <QRandomGenerator>
#include <algorithm>

AudioManager::AudioManager(QObject* parent) : QObject(parent) {
    player.setAudioOutput(&output);
    output.setVolume(float(volume01));
}

void AudioManager::setVolume01(double v01) {
    volume01 = std::clamp(v01, 0.0, 1.0);

    // UI 0.0-1.0 映射到 0.0-2.0 的软件增益
    const double boosted = std::clamp(volume01 * 2.0, 0.0, 2.0);
    output.setVolume(float(boosted));
}


void AudioManager::setAssetsRoot(const QString& assetsRoot) {
    root = assetsRoot;
}

QStringList AudioManager::scanAudioFiles(const QString& dirPath) const {
    QStringList out;
    QDir dir(dirPath);
    if (!dir.exists()) return out;

    QFileInfoList files = dir.entryInfoList(
        {"*.wav","*.WAV","*.ogg","*.OGG","*.mp3","*.MP3"},
        QDir::Files, QDir::Name);

    for (auto& fi : files) out << fi.absoluteFilePath();
    return out;
}

void AudioManager::rebuildIndex() {
    bank.clear();
    if (root.isEmpty()) return;

    const QString base = QDir(root).filePath("audio/wife");
    bank["idle"] = scanAudioFiles(QDir(base).filePath("idle"));
    bank["happy"] = scanAudioFiles(QDir(base).filePath("happy"));
    bank["angry"] = scanAudioFiles(QDir(base).filePath("angry"));
    bank["dragging"] = scanAudioFiles(QDir(base).filePath("dragging"));
    bank["hit"] = scanAudioFiles(QDir(base).filePath("hit"));

}

void AudioManager::stop() {
    player.stop();
}

void AudioManager::playRandom(const QString& category) {
    const auto list = bank.value(category);
    if (list.isEmpty()) return;

    const int idx = QRandomGenerator::global()->bounded(list.size());
    const QString path = list.at(idx);

    player.stop();
    player.setSource(QUrl::fromLocalFile(path));
    player.play();
}
