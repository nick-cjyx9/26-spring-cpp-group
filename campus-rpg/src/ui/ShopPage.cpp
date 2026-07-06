#include "ShopPage.h"
#include "ui_ShopPage.h"

#include "GameManager.h"
#include "Item.h"
#include "Shop.h"

#include <QMessageBox>

ShopPage::ShopPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::ShopPage)
{
    ui->setupUi(this);
    setWindowTitle("Shop");
    connect(ui->buyButton, &QPushButton::clicked, this, &ShopPage::onBuyClicked);
    connect(ui->sellButton, &QPushButton::clicked, this, &ShopPage::onSellClicked);
}

ShopPage::~ShopPage()
{
    delete ui;
}

void ShopPage::refresh()
{
    ui->shopList->clear();
    const auto &shop = GameManager::instance().shop();
    for (const auto &item : shop.items())
    {
        QString text = QString::fromStdString(item->name() + " - " + std::to_string(item->value()) + "g");
        ui->shopList->addItem(text);
    }

    ui->inventoryList->clear();
    const auto &inv = GameManager::instance().inventory();
    for (const auto &item : inv.items())
    {
        QString text = QString::fromStdString(item->name() + " - " + std::to_string(item->value() / 2) + "g");
        ui->inventoryList->addItem(text);
    }
}

void ShopPage::onBuyClicked()
{
    int row = ui->shopList->currentRow();
    if (row < 0)
        return;

    auto &gm = GameManager::instance();
    if (gm.shop().buy(static_cast<size_t>(row), gm.character(), gm.inventory()))
    {
        refresh();
    }
    else
    {
        QMessageBox::warning(this, "Buy", "Not enough gold or invalid item.");
    }
}

void ShopPage::onSellClicked()
{
    int row = ui->inventoryList->currentRow();
    if (row < 0)
        return;

    auto &gm = GameManager::instance();
    if (gm.shop().sell(static_cast<size_t>(row), gm.character(), gm.inventory()))
    {
        refresh();
    }
}
