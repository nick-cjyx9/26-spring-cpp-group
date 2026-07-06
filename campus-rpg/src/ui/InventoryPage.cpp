#include "InventoryPage.h"
#include "ui_InventoryPage.h"

#include "GameManager.h"
#include "Inventory.h"
#include "Item.h"

#include <QMessageBox>

InventoryPage::InventoryPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::InventoryPage)
{
    ui->setupUi(this);
    setWindowTitle("Inventory");
    connect(ui->useButton, &QPushButton::clicked, this, &InventoryPage::onUseClicked);
    connect(ui->dropButton, &QPushButton::clicked, this, &InventoryPage::onDropClicked);
}

InventoryPage::~InventoryPage()
{
    delete ui;
}

void InventoryPage::refresh()
{
    ui->listWidget->clear();
    const auto &inv = GameManager::instance().inventory();
    for (const auto &item : inv.items())
    {
        QString text = QString::fromStdString(item->name() + " [" + item->typeString() + "]");
        ui->listWidget->addItem(text);
    }
}

void InventoryPage::onUseClicked()
{
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return;

    auto &gm = GameManager::instance();
    if (gm.inventory().useItem(static_cast<size_t>(row), gm.character()))
    {
        refresh();
    }
    else
    {
        QMessageBox::warning(this, "Use Item", "Failed to use item.");
    }
}

void InventoryPage::onDropClicked()
{
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return;

    auto &gm = GameManager::instance();
    if (gm.inventory().removeItem(static_cast<size_t>(row)))
    {
        refresh();
    }
}
