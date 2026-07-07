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
    Status,
    Armory
};

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

    void initDefaultShop();
    void initDefaultQuests();
    void initDefaultEnemies();
    void initDefaultSocialLinks();
    void initDefaultPersonas();
    void initDefaultMap();
    void initDefaultEquipment();

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
    std::vector<std::shared_ptr<EquipmentItem>> equipmentDatabase_;
    EquippedGear equippedGear_;

    std::unique_ptr<engine::IScene> currentScene_;
    SceneType currentSceneType_ = SceneType::Town;

    bool isNight_ = false;
    bool shouldQuit_ = false;
    int currentSlotId_ = 1;
};
