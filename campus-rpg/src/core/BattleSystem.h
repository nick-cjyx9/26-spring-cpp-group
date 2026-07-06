#pragma once

#include "Character.h"
#include "Enemy.h"

#include <memory>
#include <string>
#include <vector>

class BattleSystem
{
public:
    BattleSystem() = default;

    void startBattle(Character &player, Enemy &enemy);
    void startBattle(Character &player, std::vector<std::unique_ptr<Enemy>> enemies);

    // Player actions.
    bool playerAttack(size_t enemyIndex = 0);
    bool playerUseSkill(size_t enemyIndex, const Skill &skill);
    bool playerUseItem(size_t inventoryIndex);
    bool playerGuard();
    bool playerSwitchPersona(std::shared_ptr<Persona> persona);
    bool playerFlee();

    // Enemy turn.
    void enemyTurn();

    bool isOver() const;
    bool playerWon() const;
    bool playerLost() const;

    Enemy *enemyAt(size_t index) const;
    const std::vector<std::unique_ptr<Enemy>> &enemies() const { return enemies_; }

    const std::vector<std::string> &log() const { return log_; }
    void clearLog() { log_.clear(); }
    void appendLog(const std::string &message);

private:
    int applyAffinityMultiplier(int rawDamage, Affinity affinity) const;

    Character *player_ = nullptr;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::string> log_;

    bool playerGuarding_ = false;
    bool fled_ = false;
};
