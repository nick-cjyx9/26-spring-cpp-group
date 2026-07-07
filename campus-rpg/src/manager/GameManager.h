#pragma once

#include "BattleSystem.h"
#include "Character.h"
#include "Inventory.h"
#include "IScene.h"
#include "QuestManager.h"
#include "Shop.h"
#include "SocialLinkManager.h"
#include "TileMap.h"

#include <functional>
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

// Fired whenever a Social Link ranks up. The UI layer registers a callback
// here (via GameManager::setRankUpCallback) to play the 奶龙笑 / 火舞 sound,
// show a rank-up banner, etc. Kept as std::function so the core/manager
// layer stays free of SFML/audio dependencies.
//   arg1: Social Link id (e.g. "sl_yosuke")
//   arg2: the new rank just reached (1..10)
using RankUpCallback = std::function<void(const std::string &socialLinkId, int newRank)>;

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

    // ---- Social Link integration ----
    // Called by DialogueScene when the player talks to an NPC.
    // Adds the daily progression points, fires any pending rank-up callbacks
    // (so the UI can play the 奶龙 laugh / fire-dance sound), recomputes
    // stat bonuses on the character, and returns the link's current-rank
    // dialogue text.
    std::string talkToNpc(const std::string &socialLinkId);

    // Recompute and apply all Social Link stat bonuses onto the character.
    // Called internally after talkToNpc / load. UI may also call it on demand.
    void recomputeSocialLinkBonuses();

    // UI layer registers a callback to be fired on every Social Link rank-up
    // (used to play the 奶龙 laugh / fire-dance sound and show a banner).
    void setRankUpCallback(RankUpCallback cb) { rankUpCallback_ = std::move(cb); }

    void initDefaultShop();
    void initDefaultQuests();
    void initDefaultEnemies();
    void initDefaultSocialLinks();
    void initSocialLinkRankData();
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

    RankUpCallback rankUpCallback_;
};
