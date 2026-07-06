#include "BattleSystem.h"

void BattleSystem::startBattle(Character &player, Enemy &enemy)
{
    player_ = &player;
    enemy_ = &enemy;
    log_.clear();
    appendLog("Battle started: " + player.name() + " vs " + enemy.name());
    appendLog(enemy.battleCry());
}

bool BattleSystem::playerAttack()
{
    if (isOver())
        return false;
    enemy_->takeDamage(player_->attack());
    appendLog(player_->name() + " attacks " + enemy_->name() +
              " for " + std::to_string(player_->attack()) + " damage.");
    return true;
}

bool BattleSystem::enemyAttack()
{
    if (isOver())
        return false;
    player_->takeDamage(enemy_->attack());
    appendLog(enemy_->name() + " attacks " + player_->name() +
              " for " + std::to_string(enemy_->attack()) + " damage.");
    return true;
}

bool BattleSystem::isOver() const
{
    return !player_->isAlive() || !enemy_->isAlive();
}

bool BattleSystem::playerWon() const
{
    return player_->isAlive() && !enemy_->isAlive();
}

void BattleSystem::appendLog(const std::string &message)
{
    log_.push_back(message);
}
