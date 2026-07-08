#pragma once

class Character;
class Inventory;
class QuestManager;
class SocialLinkManager;
class Persona;

#include <memory>
#include <string>
#include <vector>

// Metadata for a single save slot, used by the UI to list/select slots.
struct SaveSlotInfo
{
    int slotId = 0;
    bool exists = false;
    std::string characterName;
    int level = 1;
    std::string updatedAt;
};

class SaveRepository
{
public:
    SaveRepository() = default;

    // ---- Multi-slot API (preferred) ----
    bool saveAll(int slotId, const Character &character, const Inventory &inventory,
                 const std::vector<std::shared_ptr<Persona>> &personas,
                 const SocialLinkManager &socialLinks, const QuestManager &quests);
    bool loadAll(int slotId, Character &character, Inventory &inventory,
                 std::vector<std::shared_ptr<Persona>> &personas,
                 SocialLinkManager &socialLinks, QuestManager &quests);

    bool deleteSlot(int slotId);
    bool slotExists(int slotId);
    SaveSlotInfo slotInfo(int slotId);
    // Returns slot metadata for slots 1..maxSlotId (empty slots have exists=false).
    std::vector<SaveSlotInfo> listSlots(int maxSlotId = 3);
    // Returns metadata for ALL existing saves (dynamic count), ordered by slot_id.
    std::vector<SaveSlotInfo> listAllSlots();
    // Returns the next free slot id (max existing + 1, or 1 if none).
    int nextSlotId();
    // Returns the current persona id stored on the character row (empty if none).
    std::string currentPersonaId(int slotId);

    // ---- Equipment slot persistence (weapon/armor/accessory/relic) ----
    // Persist the item id currently sitting in each gear slot. Empty string
    // means the slot is empty. Stored on the character row.
    bool saveEquipmentSlots(int slotId, const std::string &weaponId,
                            const std::string &armorId,
                            const std::string &accessoryId,
                            const std::string &relicId);
    // Read back the four slot item ids. Empty string = empty slot (also the
    // fallback for legacy saves that predate these columns).
    bool loadEquipmentSlots(int slotId, std::string &weaponId,
                            std::string &armorId,
                            std::string &accessoryId,
                            std::string &relicId);

    // ---- Legacy single-slot API (delegates to slot 1, kept for compatibility) ----
    bool saveAll(const Character &character, const Inventory &inventory,
                 const std::vector<std::shared_ptr<Persona>> &personas,
                 const SocialLinkManager &socialLinks, const QuestManager &quests);
    bool loadAll(Character &character, Inventory &inventory,
                 std::vector<std::shared_ptr<Persona>> &personas,
                 SocialLinkManager &socialLinks, QuestManager &quests);

private:
    bool saveCharacter_(int slotId, const Character &character);
    bool loadCharacter_(int slotId, Character &character);
    bool saveInventory_(int slotId, const Inventory &inventory);
    bool loadInventory_(int slotId, Inventory &inventory);
    bool savePersonas_(int slotId, const std::vector<std::shared_ptr<Persona>> &personas);
    bool loadPersonas_(int slotId, std::vector<std::shared_ptr<Persona>> &personas,
                       std::string &currentPersonaId);
    bool saveSocialLinks_(int slotId, const SocialLinkManager &manager);
    bool loadSocialLinks_(int slotId, SocialLinkManager &manager);
    bool saveQuests_(int slotId, const QuestManager &manager);
    bool loadQuests_(int slotId, QuestManager &manager);
    bool saveMeta_(int slotId, const Character &character);
    bool deleteMeta_(int slotId);
};
