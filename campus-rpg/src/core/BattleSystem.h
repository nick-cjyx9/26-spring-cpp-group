#pragma once

#include "Character.h"
#include "Enemy.h"
#include "Item.h"

#include <memory>
#include <string>
#include <vector>

class BattleSystem
{
public:
    BattleSystem() = default;

    void startBattle(Character &player, Enemy &enemy);
    void startBattle(Character &player, std::vector<std::unique_ptr<Enemy>> enemies);

    // Returns true if it is currently the player's turn and the battle is ongoing.
    bool isPlayerTurn() const;

    // Player actions. These may be called only during the player's turn.
    bool playerAttack(size_t enemyIndex = 0);
    bool playerUseSkill(size_t enemyIndex, const Skill &skill);
    bool playerUseItem(std::shared_ptr<Item> item); // free action, does not end turn
    bool playerSwitchPersona(std::shared_ptr<Persona> persona);
    bool playerFlee();

    // Processes enemy turns until it is the player's turn or the battle ends.
    void processEnemyTurns();

    bool isOver() const;
    bool playerWon() const;
    bool playerLost() const;
    bool playerFled() const { return fled_; }

    Enemy *enemyAt(size_t index) const;
    const std::vector<std::unique_ptr<Enemy>> &enemies() const { return enemies_; }

    const std::vector<std::string> &log() const { return log_; }
    void clearLog() { log_.clear(); }
    void appendLog(const std::string &message);

    // Convenience: fully restore player SP (called when battle ends).
    void recoverPlayerSp();

    // Drop results populated on victory.
    const std::vector<std::string> &droppedPersonaIds() const { return droppedPersonaIds_; }

private:
    struct TurnEntry
    {
        bool isPlayer = false;
        size_t enemyIndex = 0;
        int initiative = 0;
    };

    void buildTurnOrder();
    void advanceTurn();
    void executeEnemyTurn(size_t enemyIndex);
    int applyAffinityMultiplier(int rawDamage, Affinity affinity) const;
    int physicalDamage(int attackerStrength) const;
    bool rollDodge(int attackerSpeed, int defenderSpeed) const;
    bool rollFleeSuccess(int playerSpeed, int enemySpeed) const;

    Character *player_ = nullptr;
    std::vector<std::unique_ptr<Enemy>> enemies_;
    std::vector<std::string> log_;
    std::vector<TurnEntry> turnOrder_;
    size_t currentTurnIndex_ = 0;
    size_t enemyTurnCounter_ = 0; // counts enemy actions for fixed attack patterns
    bool fled_ = false;
    std::vector<std::string> droppedPersonaIds_;
};
