#include "GameManager.h"
#include "Enemy.h"
#include "Item.h"

GameManager &GameManager::instance()
{
    static GameManager manager;
    return manager;
}

void GameManager::newGame(const std::string &playerName)
{
    character_ = Character(playerName, 100, 12, 5);
    inventory_.clear();
    shop_ = Shop();
    questManager_ = QuestManager();
    enemyTemplates_.clear();

    initDefaultShop();
    initDefaultQuests();
    initDefaultEnemies();
}

void GameManager::initDefaultShop()
{
    shop_.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "A loaf of campus bread.", 10, 15));
    shop_.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "Restores a lot of HP.", 30, 50));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_wooden_sword", "Wooden Sword",
                                                  "A training sword.", 80, 5, 0));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_leather_armor", "Leather Armor",
                                                  "Basic protection.", 60, 0, 4));
}

void GameManager::initDefaultQuests()
{
    questManager_.addQuest(Quest("quest_first", "First Steps",
                                 "Defeat your first slime.", "kill:1", 20, 30));
    questManager_.addQuest(Quest("quest_veteran", "Campus Veteran",
                                 "Defeat 3 goblins.", "kill:3", 50, 80));
}

void GameManager::initDefaultEnemies()
{
    enemyTemplates_.push_back(std::make_unique<Slime>());
    enemyTemplates_.push_back(std::make_unique<Goblin>());
    enemyTemplates_.push_back(std::make_unique<Boss>());
}

std::unique_ptr<Enemy> GameManager::createEnemy(size_t index) const
{
    if (index >= enemyTemplates_.size())
        return nullptr;
    // Because Enemy is abstract and does not have clone(), we use the known IDs here.
    const std::string &id = enemyTemplates_[index]->id();
    if (id == "enemy_slime")
        return std::make_unique<Slime>();
    if (id == "enemy_goblin")
        return std::make_unique<Goblin>();
    if (id == "enemy_boss")
        return std::make_unique<Boss>();
    return nullptr;
}
