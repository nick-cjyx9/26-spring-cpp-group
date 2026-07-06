#pragma once

#include "BattleSystem.h"

#include <QWidget>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class BattlePage;
}
QT_END_NAMESPACE

class BattlePage : public QWidget
{
    Q_OBJECT

public:
    explicit BattlePage(QWidget *parent = nullptr);
    ~BattlePage();

    void refresh();

private slots:
    void onStartClicked();
    void onAttackClicked();
    void onFleeClicked();

private:
    Ui::BattlePage *ui;
    BattleSystem battleSystem_;
    std::unique_ptr<Enemy> currentEnemy_;

    void updateLog();
    void updateEnemyInfo();
};
