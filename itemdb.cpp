#include "itemdb.h"

#include <QDir>
#include <QFile>
#include <QFileInfoList>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QPainter>
#include <algorithm>

static QPixmap normalizeFrame(const QPixmap &src, const QSize &targetSize)
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

ItemType ItemDB::parseType(const QString &s)
{
    const QString t = s.trimmed().toLower();
    if (t == "food")
        return ItemType::Food;
    if (t == "weapon")
        return ItemType::Weapon;
    if (t == "shield")
        return ItemType::Shield;
    if (t == "monster")
        return ItemType::Monster;
    return ItemType::Misc;
}

ItemDef ItemDB::loadOneItemDir(const QString &id, const QString &dirPath, const QSize &targetSize)
{
    ItemDef def;
    def.id = id;
    def.name = id;
    def.type = ItemType::Misc;

    // 1) 读 manifest（可选）
    {
        QFile f(QDir(dirPath).filePath("manifest.json"));
        if (f.exists() && f.open(QIODevice::ReadOnly))
        {
            const auto doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject())
            {
                const QJsonObject o = doc.object();

                if (o.contains("name") && o["name"].isString())
                    def.name = o["name"].toString();

                if (o.contains("type") && o["type"].isString())
                    def.type = parseType(o["type"].toString());

                if (o.contains("tags") && o["tags"].isArray())
                {
                    QJsonArray arr = o["tags"].toArray();
                    for (auto v : arr)
                        if (v.isString())
                            def.tags.push_back(v.toString());
                }

                if (o.contains("stats") && o["stats"].isObject())
                    def.stats = o["stats"].toObject();

                // audio: { "actor_use": "eat", "item_use": "apple_use", "enemy_hit": "slime_hit", ... }
                if (o.contains("audio") && o["audio"].isObject())
                {
                    const QJsonObject a = o["audio"].toObject();
                    for (auto it = a.begin(); it != a.end(); ++it)
                    {
                        if (it.value().isString())
                            def.audio.insert(it.key(), it.value().toString());
                    }
                }

                if (o.contains("frame_interval_ms") && o["frame_interval_ms"].isDouble())
                    def.frameIntervalMs = int(o["frame_interval_ms"].toDouble());
            }
        }
    }

    // 2) 读 png 帧
    QDir dir(dirPath);
    QFileInfoList files = dir.entryInfoList({"*.png", "*.PNG"}, QDir::Files, QDir::Name);
    for (const auto &fi : files)
    {
        QPixmap raw(fi.absoluteFilePath());
        QPixmap norm = normalizeFrame(raw, targetSize);
        if (!norm.isNull())
            def.frames.push_back(norm);
    }

    return def;
}

bool ItemDB::load(const QString &assetsRoot, const QSize &targetSize)
{
    items.clear();
    if (assetsRoot.isEmpty())
        return false;

    QDir itemsDir(QDir(assetsRoot).filePath("items"));
    if (!itemsDir.exists())
        return false;

    const auto dirs = itemsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const auto &d : dirs)
    {
        const QString id = d.fileName();
        const QString dirPath = d.absoluteFilePath();

        ItemDef def = loadOneItemDir(id, dirPath, targetSize);
        if (def.frames.isEmpty())
            continue; // 没帧就忽略
        items.insert(id, def);
    }

    return !items.isEmpty();
}

QVector<QString> ItemDB::itemIds() const
{
    auto keys = items.keys().toVector();
    std::sort(keys.begin(), keys.end());
    return keys;
}

const ItemDef *ItemDB::get(const QString &id) const
{
    auto it = items.find(id);
    if (it == items.end())
        return nullptr;
    return &it.value();
}
