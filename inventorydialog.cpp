#include "inventorydialog.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QIcon>
#include <QFileInfo>

InventoryDialog::InventoryDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Inventory");
    setModal(false);
    resize(360, 260);

    // 外层 layout：放一个可滚动区域，物品多了也不怕
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto *tip = new QLabel("Click an item to spawn it.", this);
    root->addWidget(tip);

    // Scroll area
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    root->addWidget(scroll, 1);

    auto *container = new QWidget(scroll);
    scroll->setWidget(container);

    auto *grid = new QGridLayout(container);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);

    // 暂时先空着，rebuildUI() 会填
    container->setLayout(grid);
}

void InventoryDialog::setItems(const QVector<ItemDef> &items)
{
    items_ = items;
    rebuildUI();
}

void InventoryDialog::rebuildUI()
{
    auto *scroll = findChild<QScrollArea *>();
    if (!scroll)
        return;
    auto *container = scroll->widget();
    if (!container)
        return;
    auto *grid = qobject_cast<QGridLayout *>(container->layout());
    if (!grid)
        return;

    // 清空旧按钮
    while (grid->count() > 0)
    {
        auto *it = grid->takeAt(0);
        if (it->widget())
            it->widget()->deleteLater();
        delete it;
    }

    const int columns = 4;
    int row = 0, col = 0;

    for (const auto &item : items_)
    {
        auto *btn = new QPushButton(item.name, container);
        btn->setMinimumSize(72, 72);
        btn->setCheckable(false);

        // 有图标就显示（图标可 later 再做）
        if (!item.iconPath.isEmpty() && QFileInfo::exists(item.iconPath))
        {
            btn->setIcon(QIcon(item.iconPath));
            btn->setIconSize(QSize(56, 56));
            btn->setText("");           //  只显示图标
            btn->setToolTip(item.name); //  悬停显示名字
        }
        else
        {
            btn->setToolTip(item.name);
        }

        // 点击发信号（以后在 WifeLabel 里接住实现“生成物品”）
        connect(btn, &QPushButton::clicked, this, [this, item]()
                { emit spawnRequested(item.id); });

        grid->addWidget(btn, row, col);

        col++;
        if (col >= columns)
        {
            col = 0;
            row++;
        }
    }

    // 下面留一点弹性空间
    grid->setRowStretch(row + 1, 1);
}
