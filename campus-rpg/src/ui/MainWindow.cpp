#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "CharacterPage.h"
#include "InventoryPage.h"
#include "ShopPage.h"
#include "QuestPage.h"
#include "BattlePage.h"
#include "GameManager.h"
#include "DatabaseManager.h"
#include "SaveRepository.h"

#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), characterPage_(new CharacterPage(this)), inventoryPage_(new InventoryPage(this)), shopPage_(new ShopPage(this)), questPage_(new QuestPage(this)), battlePage_(new BattlePage(this))
{
    ui->setupUi(this);

    // Initialize game data and persistence
    GameManager::instance().newGame("Player");
    DatabaseManager::instance().initDatabase();

    connect(ui->characterButton, &QPushButton::clicked, this, &MainWindow::onCharacterClicked);
    connect(ui->inventoryButton, &QPushButton::clicked, this, &MainWindow::onInventoryClicked);
    connect(ui->shopButton, &QPushButton::clicked, this, &MainWindow::onShopClicked);
    connect(ui->questButton, &QPushButton::clicked, this, &MainWindow::onQuestClicked);
    connect(ui->battleButton, &QPushButton::clicked, this, &MainWindow::onBattleClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::onSaveClicked);
    connect(ui->loadButton, &QPushButton::clicked, this, &MainWindow::onLoadClicked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onCharacterClicked()
{
    characterPage_->refresh();
    characterPage_->show();
}

void MainWindow::onInventoryClicked()
{
    inventoryPage_->refresh();
    inventoryPage_->show();
}

void MainWindow::onShopClicked()
{
    shopPage_->refresh();
    shopPage_->show();
}

void MainWindow::onQuestClicked()
{
    questPage_->refresh();
    questPage_->show();
}

void MainWindow::onBattleClicked()
{
    battlePage_->refresh();
    battlePage_->show();
}

void MainWindow::onSaveClicked()
{
    SaveRepository repo;
    auto &gm = GameManager::instance();
    bool ok = repo.saveAll(gm.character(), gm.inventory(), gm.questManager());
    QMessageBox::information(this, "Save", ok ? "Game saved successfully." : "Failed to save game.");
}

void MainWindow::onLoadClicked()
{
    SaveRepository repo;
    auto &gm = GameManager::instance();
    bool ok = repo.loadAll(gm.character(), gm.inventory(), gm.questManager());
    QMessageBox::information(this, "Load", ok ? "Game loaded successfully." : "Failed to load game.");
}
