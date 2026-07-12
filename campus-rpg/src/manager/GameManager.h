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
    HeroSelect,
    Status,
    Armory,
    LevelUp,
    RestConfirm,
    DebugCheat,
    PauseMenu
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
    TileMap &currentMap() { return onSecondMap_ ? *secondMap_ : *currentMap_; }
    TileMap &townMap() { return *currentMap_; }
    TileMap &schoolMap() { return *secondMap_; }
    bool onSecondMap() const { return onSecondMap_; }
    void setOnSecondMap(bool onSecond) { onSecondMap_ = onSecond; }
    BattleSystem &battleSystem() { return battleSystem_; }
    engine::IScene *currentScene() { return currentScene_.get(); }

    const std::vector<std::unique_ptr<Enemy>> &enemyTemplates() const { return enemyTemplates_; }
    std::unique_ptr<Enemy> createEnemy(size_t index) const;
    std::unique_ptr<Enemy> createEnemyFromTemplate(const std::string &id) const;
    std::shared_ptr<Persona> findPersona(const std::string &id) const;

    // Persona ownership helpers.
    bool addPersonaToPlayer(std::shared_ptr<Persona> persona);
    void setPlayerPersona(std::shared_ptr<Persona> persona);
    bool destroyPlayerPersona(const std::string &id);

    // ---- Equipment helpers (used by StatusScene / InventoryScene) ----
    // Equip an item from the inventory, removing it from the backpack and
    // placing the previously-equipped item (if any) back into the backpack.
    bool equipFromInventory(size_t inventoryIndex);
    // Unequip the item in the given slot and return it to the backpack.
    bool unequipToInventory(EquipmentSlot slot);

    bool isNight() const { return isNight_; }
    void setNight(bool night) { isNight_ = night; }

    bool shouldQuit() const { return shouldQuit_; }
    void quit() { shouldQuit_ = true; }

    const std::string &currentEnemyTextureId() const { return currentEnemyTextureId_; }
    void setCurrentEnemyTextureId(const std::string &id) { currentEnemyTextureId_ = id; }
    bool infiniteGoldEnabled() const { return infiniteGoldEnabled_; }
    void setInfiniteGoldEnabled(bool enabled);
    void toggleInfiniteGold() { setInfiniteGoldEnabled(!infiniteGoldEnabled_); }

    // ---- Social Link integration ----
    // Called by DialogueScene when the player talks to an NPC.
    // Adds the daily progression points (up to kMaxTalksPerNpc talks/day),
    // fires any pending rank-up callbacks, levels up the current Persona for
    // each rank gained, teaches any rank-reward skill, and returns the link's
    // current-rank dialogue text. Once the daily talk cap is reached, returns a
    // "no more today" message and grants nothing.
    std::string talkToNpc(const std::string &socialLinkId);

    // Deprecated: Social Link rewards are now applied as Persona levels inside
    // talkToNpc(). Kept for backward compatibility with existing call sites.
    void recomputeSocialLinkBonuses();

    // UI layer registers a callback to be fired on every Social Link rank-up
    // (used to play the 奶龙 laugh / fire-dance sound and show a banner).
    void setRankUpCallback(RankUpCallback cb) { rankUpCallback_ = std::move(cb); }

    void initDefaultShop();
    void initDefaultQuests();
    void initDefaultEnemies();
    void initDefaultSocialLinks();
    void initDefaultPersonas();
    void initDefaultMap();
    void initSecondMap();
    void initDefaultEquipment();

    // ---- NPC pool & day system ----
    // A save holds a persistent pool of kNpcPoolSize unique NPCs (random name
    // + random portrait). Each day, kNpcsPerDay of them are randomly picked to
    // appear on the town map; each can be talked to kMaxTalksPerNpc times that
    // day. Going from night back to day advances to the next day and refreshes
    // the daily NPC selection.
    struct NpcDefinition
    {
        std::string id;
        std::string name;
        std::string portraitId;
        std::string spriteId;
        std::string arcana;
    };
    int day() const { return day_; }
    void advanceDay(); // night -> day transition: day++, refresh NPCs on map.
    const std::vector<std::string> &todayNpcIds() const { return todayNpcIds_; }
    const std::vector<std::string> &todaySchoolNpcIds() const { return todaySchoolNpcIds_; }
    const NpcDefinition *findNpc(const std::string &id) const;
    // Today's talk count for a given NPC id (capped at kMaxTalksPerNpc).
    int talkCountToday(const std::string &npcId) const;
    static constexpr int kNpcPoolSize = 10;
    static constexpr int kTownNpcsPerDay = 2;
    static constexpr int kSchoolNpcsPerDay = 1;
    static constexpr int kMaxTalksPerNpc = 2;

    // ---- Equipment system ----
    struct EquippedGear
    {
        std::shared_ptr<EquipmentItem> weapon;
        std::shared_ptr<EquipmentItem> armor;
        std::shared_ptr<EquipmentItem> accessory;
        std::shared_ptr<EquipmentItem> relic;
    };

    const EquippedGear &equippedGear() const { return equippedGear_; }
    void equipItem(std::shared_ptr<EquipmentItem> item);
    void unequipItem(EquipmentSlot slot);
    const std::vector<std::shared_ptr<EquipmentItem>> &equipmentDatabase() const { return equipmentDatabase_; }

    // Hero selection (0..3), persisted in the current session.
    int selectedHeroIndex() const { return selectedHeroIndex_; }
    void setSelectedHeroIndex(int idx) { selectedHeroIndex_ = idx; }

private:
    GameManager() = default;

    // Reset all game state to defaults and seed definitions. Does NOT switch
    // scenes, so loadFromSlot() can call it before overwriting with save data.
    void seedDefaultState(const std::string &playerName);

    // NPC pool helpers.
    void generateNpcPool();        // create kNpcPoolSize random NPCs + dialogue.
    void applyNpcDialogueTemplates(); // (re)fill generic dialogue + rewards.
    void refreshDailyNpcs();       // pick kNpcsPerDay random ids for both maps, reset counts.
    void rebuildMapNpcs();         // clear non-player entities, place today's NPCs on both maps.

    Character character_;
    Inventory inventory_;
    Shop shop_;
    QuestManager questManager_;
    SocialLinkManager socialLinkManager_;
    std::unique_ptr<TileMap> currentMap_;
    std::unique_ptr<TileMap> secondMap_;
    BattleSystem battleSystem_;

    std::vector<std::unique_ptr<Enemy>> enemyTemplates_;
    std::vector<std::shared_ptr<Persona>> personas_;
    std::vector<std::shared_ptr<EquipmentItem>> equipmentDatabase_;
    EquippedGear equippedGear_;

    std::unique_ptr<engine::IScene> currentScene_;
    SceneType currentSceneType_ = SceneType::Town;

    bool isNight_ = false;
    bool onSecondMap_ = false;
    bool shouldQuit_ = false;
    bool infiniteGoldEnabled_ = false;
    int currentSlotId_ = 1;
    int selectedHeroIndex_ = 0;
    std::string currentEnemyTextureId_;

    // Day & NPC pool system
    int day_ = 1;
    std::vector<NpcDefinition> npcPool_;
    std::vector<std::string> todayNpcIds_;
    std::vector<std::string> todaySchoolNpcIds_;
    std::map<std::string, int> talkCountToday_;

    RankUpCallback rankUpCallback_;
};
