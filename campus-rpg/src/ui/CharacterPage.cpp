#include "CharacterPage.h"
#include "ui_CharacterPage.h"

#include "GameManager.h"

#include <QString>

CharacterPage::CharacterPage(QWidget *parent)
    : QWidget(parent), ui(new Ui::CharacterPage)
{
    ui->setupUi(this);
    setWindowTitle("Character");
}

CharacterPage::~CharacterPage()
{
    delete ui;
}

void CharacterPage::refresh()
{
    const auto &c = GameManager::instance().character();
    ui->nameLabel->setText(QString::fromStdString(c.name()));
    ui->levelLabel->setText(QString("Level: %1").arg(c.level()));
    ui->hpLabel->setText(QString("HP: %1 / %2").arg(c.hp()).arg(c.maxHp()));
    ui->expLabel->setText(QString("EXP: %1 / %2").arg(c.exp()).arg(c.expToNextLevel()));
    ui->goldLabel->setText(QString("Gold: %1").arg(c.gold()));
    ui->attackLabel->setText(QString("Attack: %1").arg(c.attack()));
    ui->defenseLabel->setText(QString("Defense: %1").arg(c.defense()));
}
