#pragma once
#include <QDialog>
#include <QString>

class ItemDB;

class InventoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InventoryDialog(QWidget *parent = nullptr);

    void setDB(ItemDB *db);

signals:
    void spawnRequested(const QString &itemId);

private:
    ItemDB *db_ = nullptr;
    void rebuildUI();
};
