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
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "SaveRepository.h"

enum class SceneType
{
    Title,
    Town,
    Night,
    Battle,
    Shop,
    Inventory,
    Character,
    Dialogue,
    SaveSlot,
    SocialLink,
    HeroSelect
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

    // ---- Multi-slot save API (used by TitleScene / TownScene) ----
    // Save current game state into the given slot (overwrites existing).
    bool saveToSlot(int slotId);
    // Quick-save to the slot the current session is bound to.
    bool saveCurrentSlot();
    // Load a slot into the running game. Call after newGame() so default
    // quest/social-link definitions exist for progress to attach to.
    bool loadFromSlot(int slotId);
    // Create a brand-new game with the given character id and save it to slot.
    bool createNewSave(int slotId, const std::string &characterId);
    // Create a brand-new game in the next free slot (auto-assigned).
    bool createNewSave(const std::string &characterId);
    // Delete a save slot entirely.
    bool deleteSaveSlot(int slotId);
    // Query whether a slot has save data.
    bool hasSaveSlot(int slotId);
    // List up to maxSlotId slots with metadata for the load/save-slot UI.
    std::vector<SaveSlotInfo> listSaveSlots(int maxSlotId = 3);
    // List ALL existing saves (dynamic count), ordered by slot id.
    std::vector<SaveSlotInfo> listAllSaves();
    // Next free slot id (max existing + 1, or 1 if none).
    int nextSaveSlotId();
    // The slot the current game session is bound to (1-based).
    int currentSlotId() const { return currentSlotId_; }
    void setCurrentSlotId(int slotId) { currentSlotId_ = slotId; }

    // Legacy single-slot API (delegates to slot 1 / currentSlotId_).
    void save();
    void load();

    void enterScene(SceneType type);
    // Enter the save-slot management screen. create=true opens it in Create
    // mode (start game: prompt for id, auto-assign a new slot); false opens it
    // in Load mode (load game: browse/load/delete existing saves).
    void openSaveSlots(bool create);
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

    // Hero selection (0..3), persisted in the current session.
    int selectedHeroIndex() const { return selectedHeroIndex_; }
    void setSelectedHeroIndex(int idx) { selectedHeroIndex_ = idx; }

    // ---- Time system ----
    // Day starts at 1, hour starts at 8 (8 AM). Each talk = +4h, each battle = +4h.
    // Moving 4 tiles = +1h. Talk-count refreshes at 8 AM on a new day.
    int day() const { return day_; }
    int hour() const { return hour_; }
    std::string timeString() const; // e.g. "Day 1  08:00"
    void advanceTime(int hours);
    // Whether this NPC can still grant Social Link points today (max 3 talks/day).
    bool canGainPoints(const std::string &npcId) const;
    int talkCountToday(const std::string &npcId) const;
    static constexpr int kMaxTalksPerNpc = 3;
    static constexpr int kTalkCostHours = 4;
    static constexpr int kBattleCostHours = 4;
    static constexpr int kTilesPerHour = 4;
    static constexpr int kDayStartHour = 8;

private:
    GameManager() = default;

    // Reset all game state to defaults and seed definitions. Does NOT switch
    // scenes, so loadFromSlot() can call it before overwriting with save data.
    void seedDefaultState(const std::string &playerName);

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
    int currentSlotId_ = 1;
    int selectedHeroIndex_ = 0;

    // Time system
    int day_ = 1;
    int hour_ = kDayStartHour;
    int lastRefreshDay_ = 1;
    std::map<std::string, int> talkCountToday_;

    RankUpCallback rankUpCallback_;
};
