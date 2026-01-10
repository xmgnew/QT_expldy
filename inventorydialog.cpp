#include "inventorydialog.h"
#include "itemdb.h"
#include "animateditembutton.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QLabel>

InventoryDialog::InventoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Inventory");
    setModal(false);
    resize(360, 260);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto* tip = new QLabel("Click an item to spawn it.", this);
    root->addWidget(tip);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    root->addWidget(scroll, 1);

    auto* container = new QWidget(scroll);
    scroll->setWidget(container);

    auto* grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);
    container->setLayout(grid);
}

void InventoryDialog::setDB(ItemDB* db)
{
    db_ = db;
    rebuildUI();
}

void InventoryDialog::rebuildUI()
{
    auto* scroll = findChild<QScrollArea*>();
    if (!scroll) return;
    auto* container = scroll->widget();
    if (!container) return;
    auto* grid = qobject_cast<QGridLayout*>(container->layout());
    if (!grid) return;

    while (grid->count() > 0) {
        auto* it = grid->takeAt(0);
        if (it->widget()) it->widget()->deleteLater();
        delete it;
    }

    if (!db_) return;

    const auto ids = db_->itemIds();
    const int columns = 4;
    int row = 0, col = 0;

    for (const auto& id : ids)
    {
        const ItemDef* def = db_->get(id);
        if (!def) continue;

        auto* btn = new AnimatedItemButton(def->id, def->frames, def->frameIntervalMs, container);
        btn->setToolTip(def->name);

        connect(btn, &QPushButton::clicked, this, [this, id]() {
            emit spawnRequested(id);
        });

        grid->addWidget(btn, row, col);

        col++;
        if (col >= columns) { col = 0; row++; }
    }

    grid->setRowStretch(row + 1, 1);
}
