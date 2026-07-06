#include "BattlePage.h"
#include "ui_BattlePage.h"

#include "GameManager.h"

#include <QMessageBox>
#include <QString>

BattlePage::BattlePage(QWidget *parent)
    : QWidget(parent), ui(new Ui::BattlePage)
{
    ui->setupUi(this);
    setWindowTitle("Battle");
    connect(ui->startButton, &QPushButton::clicked, this, &BattlePage::onStartClicked);
    connect(ui->attackButton, &QPushButton::clicked, this, &BattlePage::onAttackClicked);
    connect(ui->fleeButton, &QPushButton::clicked, this, &BattlePage::onFleeClicked);
}

BattlePage::~BattlePage()
{
    delete ui;
}

void BattlePage::refresh()
{
    ui->enemyList->clear();
    const auto &templates = GameManager::instance().enemyTemplates();
    for (const auto &enemy : templates)
    {
        ui->enemyList->addItem(QString::fromStdString(enemy->name()));
    }
    ui->logText->clear();
    updateEnemyInfo();
}

void BattlePage::onStartClicked()
{
    int row = ui->enemyList->currentRow();
    if (row < 0)
        return;

    currentEnemy_ = GameManager::instance().createEnemy(static_cast<size_t>(row));
    if (!currentEnemy_)
        return;

    battleSystem_.startBattle(GameManager::instance().character(), *currentEnemy_);
    updateLog();
    updateEnemyInfo();
}

void BattlePage::onAttackClicked()
{
    if (!currentEnemy_)
        return;

    battleSystem_.playerAttack();
    if (!battleSystem_.isOver())
    {
        battleSystem_.enemyAttack();
    }

    updateLog();
    updateEnemyInfo();

    if (battleSystem_.isOver())
    {
        auto &gm = GameManager::instance();
        if (battleSystem_.playerWon())
        {
            gm.character().gainExp(currentEnemy_->rewardExp());
            gm.character().addGold(currentEnemy_->rewardGold());
            QMessageBox::information(this, "Victory",
                                     QString("You won! Gained %1 EXP and %2 gold.")
                                         .arg(currentEnemy_->rewardExp())
                                         .arg(currentEnemy_->rewardGold()));
        }
        else
        {
            QMessageBox::information(this, "Defeat", "You were defeated...");
        }
        currentEnemy_.reset();
    }
}

void BattlePage::onFleeClicked()
{
    if (!currentEnemy_)
        return;
    battleSystem_.clearLog();
    battleSystem_.appendLog("You fled from battle.");
    updateLog();
    currentEnemy_.reset();
    updateEnemyInfo();
}

void BattlePage::updateLog()
{
    ui->logText->clear();
    for (const auto &line : battleSystem_.log())
    {
        ui->logText->append(QString::fromStdString(line));
    }
}

void BattlePage::updateEnemyInfo()
{
    if (!currentEnemy_)
    {
        ui->enemyInfoLabel->setText("No active battle.");
        return;
    }
    ui->enemyInfoLabel->setText(QString("%1: HP %2 / %3")
                                    .arg(QString::fromStdString(currentEnemy_->name()))
                                    .arg(currentEnemy_->hp())
                                    .arg(currentEnemy_->maxHp()));
}
