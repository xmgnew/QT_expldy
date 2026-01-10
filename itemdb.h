#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
#include <QPixmap>
#include <QHash>
#include <QJsonObject>

enum class ItemType { Food, Weapon, Shield, Monster, Misc };

struct ItemDef {
    QString id;                  // 文件夹名
    QString name;                // manifest.name 或 id
    ItemType type = ItemType::Misc;
    QStringList tags;            // manifest.tags
    QJsonObject stats;           // manifest.stats（先存 JSON 方便扩展）
    // manifest.audio：事件 -> category（不限定 key，阶段2会用到 actor_use/item_use/enemy_* 等）
    QHash<QString, QString> audio;
    QVector<QPixmap> frames;     // 已缩放到 targetSize 的帧
    int frameIntervalMs = 120;   // manifest.frame_interval_ms 或默认
};

class ItemDB {
public:
    bool load(const QString& assetsRoot, const QSize& targetSize);

    QVector<QString> itemIds() const;      // 已排序
    const ItemDef* get(const QString& id) const;

private:
    QHash<QString, ItemDef> items;

    static ItemType parseType(const QString& s);
    static ItemDef loadOneItemDir(const QString& id, const QString& dirPath, const QSize& targetSize);
};
