#pragma once

#include "Character.h"
#include "Enemy.h"

#include <string>
#include <vector>

class BattleSystem
{
public:
    BattleSystem() = default;

    void startBattle(Character &player, Enemy &enemy);
    bool playerAttack();
    bool enemyAttack();
    bool isOver() const;
    bool playerWon() const;

    const std::vector<std::string> &log() const { return log_; }
    void clearLog() { log_.clear(); }

    void appendLog(const std::string &message);

private:
    Character *player_ = nullptr;
    Enemy *enemy_ = nullptr;
    std::vector<std::string> log_;
};
