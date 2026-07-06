#pragma once

#include "BattleSystem.h"
#include "Character.h"
#include "Inventory.h"
#include "IScene.h"
#include "QuestManager.h"
#include "Shop.h"
#include "SocialLinkManager.h"
#include "TileMap.h"

#include <memory>
#include <string>
#include <vector>

enum class SceneType
{
    Title,
    Town,
    Night,
    Battle,
    Shop,
    Inventory,
    Character,
    Dialogue
};

class GameManager
{
public:
    static GameManager &instance();

    void newGame(const std::string &playerName);

    void save();
    void load();

    void enterScene(SceneType type);
    SceneType currentSceneType() const { return currentSceneType_; }

    Character &character() { return character_; }
    Inventory &inventory() { return inventory_; }
    Shop &shop() { return shop_; }
    QuestManager &questManager() { return questManager_; }
    SocialLinkManager &socialLinkManager() { return socialLinkManager_; }
    TileMap &currentMap() { return *currentMap_; }
    BattleSystem &battleSystem() { return battleSystem_; }
    engine::IScene *currentScene() { return currentScene_.get(); }

    const std::vector<std::unique_ptr<Enemy>> &enemyTemplates() const { return enemyTemplates_; }
    std::unique_ptr<Enemy> createEnemy(size_t index) const;
    std::unique_ptr<Enemy> createEnemyFromTemplate(const std::string &id) const;
    std::shared_ptr<Persona> findPersona(const std::string &id) const;

    bool isNight() const { return isNight_; }
    void setNight(bool night) { isNight_ = night; }

    bool shouldQuit() const { return shouldQuit_; }
    void quit() { shouldQuit_ = true; }

    void initDefaultShop();
    void initDefaultQuests();
    void initDefaultEnemies();
    void initDefaultSocialLinks();
    void initDefaultPersonas();
    void initDefaultMap();

private:
    GameManager() = default;

    Character character_;
    Inventory inventory_;
    Shop shop_;
    QuestManager questManager_;
    SocialLinkManager socialLinkManager_;
    std::unique_ptr<TileMap> currentMap_;
    BattleSystem battleSystem_;

    std::vector<std::unique_ptr<Enemy>> enemyTemplates_;
    std::vector<std::shared_ptr<Persona>> personas_;

    std::unique_ptr<engine::IScene> currentScene_;
    SceneType currentSceneType_ = SceneType::Town;

    bool isNight_ = false;
    bool shouldQuit_ = false;
};
