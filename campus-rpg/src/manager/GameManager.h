#pragma once

#include "BattleSystem.h"
#include "Character.h"
#include "Inventory.h"
#include "QuestManager.h"
#include "Shop.h"

#include <memory>
#include <vector>

class GameManager
{
public:
    static GameManager &instance();

    void newGame(const std::string &playerName);

    Character &character() { return character_; }
    Inventory &inventory() { return inventory_; }
    Shop &shop() { return shop_; }
    QuestManager &questManager() { return questManager_; }

    const std::vector<std::unique_ptr<Enemy>> &enemyTemplates() const { return enemyTemplates_; }
    std::unique_ptr<Enemy> createEnemy(size_t index) const;

    void initDefaultShop();
    void initDefaultQuests();
    void initDefaultEnemies();

private:
    GameManager() = default;

    Character character_;
    Inventory inventory_;
    Shop shop_;
    QuestManager questManager_;
    std::vector<std::unique_ptr<Enemy>> enemyTemplates_;
};
