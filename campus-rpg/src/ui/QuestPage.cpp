#include "QuestPage.h"
#include "ui_QuestPage.h"

#include "GameManager.h"
#include "Quest.h"
#include "QuestManager.h"

#include <QMessageBox>
#include <QString>

QuestPage::QuestPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::QuestPage)
{
    ui->setupUi(this);
    setWindowTitle("Quests");
    connect(ui->acceptButton, &QPushButton::clicked, this, &QuestPage::onAcceptClicked);
    connect(ui->completeButton, &QPushButton::clicked, this, &QuestPage::onCompleteClicked);
    connect(ui->rewardButton, &QPushButton::clicked, this, &QuestPage::onRewardClicked);
}

QuestPage::~QuestPage()
{
    delete ui;
}

void QuestPage::refresh()
{
    populateList();
}

void QuestPage::populateList()
{
    ui->listWidget->clear();
    auto &qm = GameManager::instance().questManager();
    for (auto *q : qm.availableQuests())
    {
        ui->listWidget->addItem(QString("[Available] %1").arg(QString::fromStdString(q->name())));
    }
    for (auto *q : qm.acceptedQuests())
    {
        ui->listWidget->addItem(QString("[Active] %1").arg(QString::fromStdString(q->name())));
    }
    for (auto *q : qm.completedQuests())
    {
        ui->listWidget->addItem(QString("[Done] %1").arg(QString::fromStdString(q->name())));
    }
}

void QuestPage::onAcceptClicked()
{
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return;

    auto &qm = GameManager::instance().questManager();
    auto available = qm.availableQuests();
    if (row < static_cast<int>(available.size()))
    {
        qm.acceptQuest(available[row]->id());
        populateList();
    }
}

void QuestPage::onCompleteClicked()
{
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return;

    auto &qm = GameManager::instance().questManager();
    auto accepted = qm.acceptedQuests();
    if (row < static_cast<int>(accepted.size()))
    {
        if (qm.completeQuest(accepted[row]->id()))
        {
            populateList();
        }
        else
        {
            QMessageBox::information(this, "Quest", "Quest condition not yet met.");
        }
    }
}

void QuestPage::onRewardClicked()
{
    int row = ui->listWidget->currentRow();
    if (row < 0)
        return;

    auto &gm = GameManager::instance();
    auto completed = gm.questManager().completedQuests();
    if (row < static_cast<int>(completed.size()))
    {
        if (gm.questManager().rewardQuest(completed[row]->id()))
        {
            gm.character().addGold(completed[row]->rewardGold());
            gm.character().gainExp(completed[row]->rewardExp());
            populateList();
        }
    }
}
