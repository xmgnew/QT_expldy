#pragma once
#include <QDialog>
#include <QString>
#include <QVector>

class InventoryDialog : public QDialog {
    Q_OBJECT
public:
    struct ItemDef {
        QString id;        // 逻辑ID: "food_apple"
        QString name;      // 显示名: "Apple"
        QString iconPath;  // 图标路径（可空）
        QString category;  // 类别（先留着，后面做筛选/分页用）
    };

    explicit InventoryDialog(QWidget* parent = nullptr);

    void setItems(const QVector<ItemDef>& items);

signals:
    void spawnRequested(const QString& itemId);  // 点击某个物品

private:
    QVector<ItemDef> items_;
    void rebuildUI();
};
