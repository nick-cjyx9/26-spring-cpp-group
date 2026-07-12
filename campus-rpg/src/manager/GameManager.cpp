#include "GameManager.h"
#include "Enemy.h"
#include "Item.h"
#include "Persona.h"
#include "SaveRepository.h"
#include "DatabaseManager.h"
#include "TownMapData.h"

#include "TitleScene.h"
#include "TownScene.h"
#include "NightScene.h"
#include "BattleScene.h"
#include "ShopScene.h"
#include "InventoryScene.h"
#include "CharacterScene.h"
#include "DialogueScene.h"
#include "SaveSlotScene.h"
#include "SocialLinkScene.h"
#include "HeroSelectScene.h"
#include "StatusScene.h"
#include "ArmoryScene.h"
#include "LevelUpScene.h"
#include "RestConfirmScene.h"
#include "DebugCheatScene.h"

#include <algorithm>
#include <array>
#include <map>
#include <random>

namespace
{
    engine::Vec2 randomSpawnInZones(const std::vector<engine::Rect> &zones, std::mt19937 &rng)
    {
        if (zones.empty())
            return {400.0f, 300.0f};
        std::uniform_int_distribution<size_t> zoneDist(0, zones.size() - 1);
        const auto &z = zones[zoneDist(rng)];
        std::uniform_real_distribution<float> xDist(z.x + 16.0f, z.x + z.width - 16.0f);
        std::uniform_real_distribution<float> yDist(z.y + 16.0f, z.y + z.height - 16.0f);
        return {xDist(rng), yDist(rng)};
    }
} // namespace

GameManager &GameManager::instance()
{
    static GameManager manager;
    return manager;
}

void GameManager::seedDefaultState(const std::string &playerName)
{
    character_ = Character(playerName, 100, 50);
    inventory_ = Inventory(20);
    shop_ = Shop();
    questManager_ = QuestManager();
    socialLinkManager_ = SocialLinkManager();
    enemyTemplates_.clear();
    personas_.clear();
    isNight_ = false;
    infiniteGoldEnabled_ = false;

    // Reset day & NPC pool system
    day_ = 1;
    talkCountToday_.clear();
    todayNpcIds_.clear();
    todaySchoolNpcIds_.clear();
    npcPool_.clear();
    onSecondMap_ = false;

    initDefaultPersonas();
    initDefaultShop();
    initDefaultQuests();
    initDefaultEnemies();
    initDefaultSocialLinks(); // builds the 10-NPC pool + dialogue
    refreshDailyNpcs();        // pick today's 2+2 NPCs, reset talk counts
    initDefaultMap();          // place player + today's NPCs on town map
    initSecondMap();           // build school map + today's NPCs
    initDefaultEquipment();

    // Start with a fixed Fool Persona. The full Persona registry is available
    // for shop purchases and battle drops, but not owned at start.
    character_.clearOwnedPersonas();
    auto starter = findPersona("persona_orpheus");
    if (starter)
        setPlayerPersona(starter);

    recomputeSocialLinkBonuses();
}

void GameManager::newGame(const std::string &playerName)
{
    seedDefaultState(playerName);
    enterScene(SceneType::Title);
}

bool GameManager::saveToSlot(int slotId)
{
    // Find player position from the current map.
    float posX = 0.0f;
    float posY = 0.0f;
    TileMap &map = currentMap();
    for (const auto &e : map.entities())
    {
        if (e && e->type() == "player")
        {
            posX = e->position().x;
            posY = e->position().y;
            break;
        }
    }

    std::vector<std::shared_ptr<EquipmentItem>> equipped = {
        equippedGear_.weapon, equippedGear_.armor, equippedGear_.accessory, equippedGear_.relic};

    SaveRepository repo;
    bool ok = repo.saveAll(slotId, character_, inventory_, character_.ownedPersonas(),
                           socialLinkManager_, questManager_, day_,
                           posX, posY, isNight_, onSecondMap_, equipped);
    if (ok)
        currentSlotId_ = slotId;
    return ok;
}

bool GameManager::saveCurrentSlot()
{
    return saveToSlot(currentSlotId_);
}

bool GameManager::loadFromSlot(int slotId)
{
    SaveRepository repo;
    if (!repo.slotExists(slotId))
        return false;

    // Seed defaults so quest / social-link definitions exist for progress to
    // attach to. The save data then overwrites character/inventory/persona/
    // quest/social-link state. The name is replaced by the save's character.
    seedDefaultState("Player");

    // Snapshot skills from the seeded default Personas (by id). The data layer
    // persists skill ids but cannot reconstruct Skill objects without a
    // registry, so we re-apply the seeded skills to matching personas after
    // load. This restores gameplay state for the default personas.
    std::map<std::string, std::vector<std::shared_ptr<Skill>>> skillSnapshot;
    std::vector<std::shared_ptr<Persona>> defaultPersonaRegistry = personas_;
    for (const auto &p : defaultPersonaRegistry)
    {
        if (!p)
            continue;
        std::vector<std::shared_ptr<Skill>> skills(p->skills().begin(), p->skills().end());
        skillSnapshot[p->id()] = std::move(skills);
    }

    int loadedDay = 1;
    float loadedPosX = 0.0f;
    float loadedPosY = 0.0f;
    bool loadedIsNight = false;
    bool loadedOnSecondMap = false;
    std::vector<std::unique_ptr<Item>> loadedEquippedGear;
    if (!repo.loadAll(slotId, character_, inventory_, personas_,
                      socialLinkManager_, questManager_, &loadedDay,
                      &loadedPosX, &loadedPosY, &loadedIsNight, &loadedOnSecondMap,
                      &loadedEquippedGear))
    {
        // Load failed: leave the seeded default state intact so the game is
        // still in a runnable state.
        return false;
    }

    // Legacy saves did not persist Item::textureId(), so equipment restored from
    // those saves can still have correct stats but no icon in StatusScene.
    // Rehydrate equipment textures from the seeded equipment database by item id.
    auto restoreEquipmentTexture = [this](EquipmentItem &item)
    {
        if (!item.textureId().empty())
            return;
        auto it = std::find_if(equipmentDatabase_.begin(), equipmentDatabase_.end(),
                               [&item](const std::shared_ptr<EquipmentItem> &candidate)
                               {
                                   return candidate && candidate->id() == item.id();
                               });
        if (it != equipmentDatabase_.end() && *it)
            item.setTextureId((*it)->textureId());
    };
    for (const auto &item : inventory_.items())
    {
        if (auto *eq = dynamic_cast<EquipmentItem *>(item.get()))
            restoreEquipmentTexture(*eq);
    }
    for (const auto &item : loadedEquippedGear)
    {
        if (auto *eq = dynamic_cast<EquipmentItem *>(item.get()))
            restoreEquipmentTexture(*eq);
    }

    // Re-apply skills to the reconstructed owned Personas (loaded ones start empty),
    // then merge in default unowned Personas so shop/drop lookup still works.
    std::vector<std::string> loadedOwnedPersonaIds;
    for (auto &p : personas_)
    {
        if (!p)
            continue;
        loadedOwnedPersonaIds.push_back(p->id());
        auto it = skillSnapshot.find(p->id());
        if (it != skillSnapshot.end())
        {
            for (const auto &s : it->second)
                p->learnSkill(s);
        }
    }
    for (const auto &defaultPersona : defaultPersonaRegistry)
    {
        if (defaultPersona && !findPersona(defaultPersona->id()))
            personas_.push_back(defaultPersona);
    }

    currentSlotId_ = slotId;
    day_ = loadedDay;
    isNight_ = loadedIsNight;
    onSecondMap_ = loadedOnSecondMap;

    // Restore player position on the current map.
    TileMap &map = currentMap();
    for (const auto &e : map.entities())
    {
        if (e && e->type() == "player")
        {
            static_cast<PlayerEntity *>(e.get())->setPosition({loadedPosX, loadedPosY});
            break;
        }
    }

    // Restore the currently-equipped persona from the save.
    std::string currentId = repo.currentPersonaId(slotId);
    if (!currentId.empty())
    {
        auto p = findPersona(currentId);
        if (p)
            character_.setPersona(p);
    }

    // Sync only the saved owned Personas after load; the rest of personas_ is
    // the global registry used by shop/drop lookup.
    character_.clearOwnedPersonas();
    for (const auto &ownedId : loadedOwnedPersonaIds)
    {
        auto p = findPersona(ownedId);
        if (p)
            character_.addPersona(p);
    }

    // Restore equipped gear objects and recompute equipment bonuses from slots.
    equippedGear_ = EquippedGear();
    character_.setEquipmentBonuses(0, 0, 0);
    for (auto &item : loadedEquippedGear)
    {
        auto *eq = dynamic_cast<EquipmentItem *>(item.get());
        if (!eq)
            continue;
        auto shared = std::shared_ptr<EquipmentItem>(static_cast<EquipmentItem *>(item.release()));
        switch (shared->slot())
        {
        case EquipmentSlot::Weapon:
            equippedGear_.weapon = shared;
            break;
        case EquipmentSlot::Armor:
            equippedGear_.armor = shared;
            break;
        case EquipmentSlot::Accessory:
            equippedGear_.accessory = shared;
            break;
        case EquipmentSlot::Relic:
            equippedGear_.relic = shared;
            break;
        default:
            break;
        }
        character_.equip(shared);
    }

    // Names/portraits were just overwritten from the save; rebuild the generic
    // dialogue so the embedded names match the loaded identity.
    applyNpcDialogueTemplates();
    recomputeSocialLinkBonuses();

    return true;
}

bool GameManager::createNewSave(int slotId, const std::string &characterId)
{
    seedDefaultState(characterId);
    currentSlotId_ = slotId;
    return saveToSlot(slotId);
}

bool GameManager::createNewSave(const std::string &characterId)
{
    return createNewSave(nextSaveSlotId(), characterId);
}

bool GameManager::deleteSaveSlot(int slotId)
{
    SaveRepository repo;
    return repo.deleteSlot(slotId);
}

bool GameManager::hasSaveSlot(int slotId)
{
    SaveRepository repo;
    return repo.slotExists(slotId);
}

std::vector<SaveSlotInfo> GameManager::listSaveSlots(int maxSlotId)
{
    SaveRepository repo;
    return repo.listSlots(maxSlotId);
}

std::vector<SaveSlotInfo> GameManager::listAllSaves()
{
    SaveRepository repo;
    return repo.listAllSlots();
}

int GameManager::nextSaveSlotId()
{
    SaveRepository repo;
    return repo.nextSlotId();
}

void GameManager::save()
{
    saveToSlot(currentSlotId_);
}

void GameManager::load()
{
    loadFromSlot(currentSlotId_);
}

void GameManager::enterScene(SceneType type)
{
    currentSceneType_ = type;
    switch (type)
    {
    case SceneType::Title:
        currentScene_ = std::make_unique<TitleScene>();
        break;
    case SceneType::Town:
        currentScene_ = std::make_unique<TownScene>();
        break;
    case SceneType::Night:
        currentScene_ = std::make_unique<NightScene>();
        break;
    case SceneType::Battle:
        currentScene_ = std::make_unique<BattleScene>();
        break;
    case SceneType::Shop:
        currentScene_ = std::make_unique<ShopScene>();
        break;
    case SceneType::Inventory:
        currentScene_ = std::make_unique<InventoryScene>();
        break;
    case SceneType::Character:
        currentScene_ = std::make_unique<CharacterScene>();
        break;
    case SceneType::Dialogue:
        currentScene_ = std::make_unique<DialogueScene>();
        break;
    case SceneType::SaveSlot:
        currentScene_ = std::make_unique<SaveSlotScene>(SaveSlotScene::Mode::Load);
        break;
    case SceneType::SocialLink:
        currentScene_ = std::make_unique<SocialLinkScene>();
        break;
    case SceneType::HeroSelect:
        currentScene_ = std::make_unique<HeroSelectScene>();
        break;
    case SceneType::Status:
        currentScene_ = std::make_unique<StatusScene>();
        break;
    case SceneType::Armory:
        currentScene_ = std::make_unique<ArmoryScene>();
        break;
    case SceneType::LevelUp:
        currentScene_ = std::make_unique<LevelUpScene>();
        break;
    case SceneType::RestConfirm:
        currentScene_ = std::make_unique<RestConfirmScene>(isNight_);
        break;
    case SceneType::DebugCheat:
        currentScene_ = std::make_unique<DebugCheatScene>();
        break;
    }
}

void GameManager::setInfiniteGoldEnabled(bool enabled)
{
    infiniteGoldEnabled_ = enabled;
    if (infiniteGoldEnabled_ && character_.gold() < 999999)
        character_.setGold(999999);
}

void GameManager::openSaveSlots(bool create)
{
    currentSceneType_ = SceneType::SaveSlot;
    currentScene_ = std::make_unique<SaveSlotScene>(
        create ? SaveSlotScene::Mode::Create : SaveSlotScene::Mode::Load);
}

std::unique_ptr<Enemy> GameManager::createEnemy(size_t index) const
{
    if (index >= enemyTemplates_.size())
        return nullptr;
    return enemyTemplates_[index]->clone();
}

std::unique_ptr<Enemy> GameManager::createEnemyFromTemplate(const std::string &id) const
{
    for (const auto &enemy : enemyTemplates_)
    {
        if (enemy && enemy->id() == id)
        {
            auto cloned = enemy->clone();
            if (cloned)
            {
                // Enemies on the second (school) night map are tougher.
                double boost = onSecondMap_ ? 1.3 : 1.0;
                cloned->scaleToLevel(character_.level(), boost);
            }
            return cloned;
        }
    }
    return nullptr;
}

std::shared_ptr<Persona> GameManager::findPersona(const std::string &id) const
{
    for (const auto &p : personas_)
    {
        if (p && p->id() == id)
            return p;
    }
    return nullptr;
}

bool GameManager::addPersonaToPlayer(std::shared_ptr<Persona> persona)
{
    if (!persona)
        return false;
    bool added = character_.addPersona(persona);
    if (added && !findPersona(persona->id()))
        personas_.push_back(persona);
    return added;
}

void GameManager::setPlayerPersona(std::shared_ptr<Persona> persona)
{
    if (!persona)
        return;
    if (!character_.ownsPersona(persona->id()))
        addPersonaToPlayer(persona);
    if (character_.ownsPersona(persona->id()))
        character_.setPersona(persona);
}

bool GameManager::destroyPlayerPersona(const std::string &id)
{
    if (character_.ownedPersonas().size() <= 1)
        return false;
    Persona *current = character_.currentPersona();
    if (current && current->id() == id)
        return false;
    return character_.removePersona(id);
}

bool GameManager::unequipToInventory(EquipmentSlot slot)
{
    std::shared_ptr<EquipmentItem> item;
    switch (slot)
    {
    case EquipmentSlot::Weapon:
        item = equippedGear_.weapon;
        break;
    case EquipmentSlot::Armor:
        item = equippedGear_.armor;
        break;
    case EquipmentSlot::Accessory:
        item = equippedGear_.accessory;
        break;
    case EquipmentSlot::Relic:
        item = equippedGear_.relic;
        break;
    default:
        return false;
    }

    if (!item || inventory_.isFull())
        return false;

    unequipItem(slot);
    return inventory_.addItem(item->clone());
}

void GameManager::initDefaultPersonas()
{
    struct PersonaSeed
    {
        const char *id;
        const char *name;
        const char *arcana;
        int level;
        int strength;
        int magic;
        int speed;
        Element resist;
        Element weak;
        const char *skill1;
        const char *skill2;
        const char *unlock1;
        const char *unlock2;
    };

    auto skill = [](const std::string &id) -> std::shared_ptr<Skill>
    {
        static const std::map<std::string, std::tuple<const char *, Element, int, int, SkillCostType, bool>> kSkills = {
            {"skill_agi", {"Agi", Element::Fire, 6, 4, SkillCostType::SP, false}},
            {"skill_agilao", {"Agilao", Element::Fire, 12, 8, SkillCostType::SP, false}},
            {"skill_maragi", {"Maragi", Element::Fire, 20, 12, SkillCostType::SP, false}},
            {"skill_bufu", {"Bufu", Element::Ice, 6, 4, SkillCostType::SP, false}},
            {"skill_bufula", {"Bufula", Element::Ice, 12, 8, SkillCostType::SP, false}},
            {"skill_zio", {"Zio", Element::Electric, 6, 4, SkillCostType::SP, false}},
            {"skill_zionga", {"Zionga", Element::Electric, 12, 8, SkillCostType::SP, false}},
            {"skill_mazionga", {"Mazionga", Element::Electric, 20, 14, SkillCostType::SP, false}},
            {"skill_garu", {"Garu", Element::Wind, 6, 4, SkillCostType::SP, false}},
            {"skill_garula", {"Garula", Element::Wind, 12, 8, SkillCostType::SP, false}},
            {"skill_magaru", {"Magaru", Element::Wind, 18, 12, SkillCostType::SP, false}},
            {"skill_hama", {"Hama", Element::Light, 10, 8, SkillCostType::SP, false}},
            {"skill_mahama", {"Mahama", Element::Light, 18, 14, SkillCostType::SP, false}},
            {"skill_mudo", {"Mudo", Element::Dark, 10, 8, SkillCostType::SP, false}},
            {"skill_mamudo", {"Mamudo", Element::Dark, 18, 14, SkillCostType::SP, false}},
            {"skill_cleave", {"Cleave", Element::Physical, 8, 0, SkillCostType::HP, false}},
            {"skill_slash", {"Slash", Element::Physical, 10, 0, SkillCostType::HP, false}},
            {"skill_mighty_swing", {"Mighty Swing", Element::Physical, 20, 0, SkillCostType::HP, false}},
            {"skill_dia", {"Dia", Element::Almighty, 15, 6, SkillCostType::SP, true}},
            {"skill_diarama", {"Diarama", Element::Almighty, 35, 12, SkillCostType::SP, true}},
            {"skill_megidola", {"Megidola", Element::Almighty, 25, 18, SkillCostType::SP, false}},
        };
        auto it = kSkills.find(id);
        if (it == kSkills.end())
            return nullptr;
        const auto &[name, element, power, cost, costType, healing] = it->second;
        return std::make_shared<Skill>(id, name, element, power, cost, costType, healing);
    };

    static const std::array<PersonaSeed, 44> kPersonas = {{
        {"persona_orpheus", "Orpheus", "Fool", 1, 5, 5, 5, Element::Fire, Element::Ice, "skill_agi", "skill_dia", "skill_agilao", "skill_diarama"},
        {"persona_sun_wukong", "Sun Wukong", "Fool", 4, 9, 6, 9, Element::Physical, Element::Ice, "skill_slash", "skill_garu", "skill_mighty_swing", "skill_megidola"},
        {"persona_pixie", "Pixie", "Magician", 1, 4, 7, 6, Element::Wind, Element::Electric, "skill_garu", "skill_dia", "skill_garula", "skill_diarama"},
        {"persona_merlin", "Merlin", "Magician", 4, 5, 12, 7, Element::Electric, Element::Dark, "skill_zio", "skill_diarama", "skill_zionga", "skill_megidola"},
        {"persona_kikuri_hime", "Kikuri-Hime", "Priestess", 1, 4, 8, 5, Element::Light, Element::Dark, "skill_hama", "skill_dia", "skill_bufula", "skill_diarama"},
        {"persona_chang_e", "Chang'e", "Priestess", 4, 5, 12, 7, Element::Ice, Element::Fire, "skill_bufu", "skill_diarama", "skill_bufula", "skill_mahama"},
        {"persona_gaia", "Gaia", "Empress", 2, 5, 8, 4, Element::Wind, Element::Fire, "skill_garu", "skill_dia", "skill_garula", "skill_diarama"},
        {"persona_nuwa", "Nuwa", "Empress", 4, 6, 12, 6, Element::Light, Element::Dark, "skill_hama", "skill_diarama", "skill_mahama", "skill_megidola"},
        {"persona_ares", "Ares", "Emperor", 2, 8, 4, 5, Element::Physical, Element::Ice, "skill_cleave", "skill_agi", "skill_slash", "skill_agilao"},
        {"persona_odin", "Odin", "Emperor", 4, 10, 8, 6, Element::Electric, Element::Wind, "skill_zionga", "skill_slash", "skill_mazionga", "skill_mighty_swing"},
        {"persona_xuanwu", "Xuanwu", "Hierophant", 2, 5, 7, 5, Element::Ice, Element::Fire, "skill_bufu", "skill_dia", "skill_bufula", "skill_diarama"},
        {"persona_thoth", "Thoth", "Hierophant", 4, 5, 12, 7, Element::Light, Element::Physical, "skill_hama", "skill_zio", "skill_mahama", "skill_megidola"},
        {"persona_eros", "Eros", "Lovers", 1, 5, 6, 7, Element::Wind, Element::Electric, "skill_garu", "skill_dia", "skill_garula", "skill_diarama"},
        {"persona_aphrodite", "Aphrodite", "Lovers", 4, 4, 12, 8, Element::Light, Element::Dark, "skill_hama", "skill_diarama", "skill_mahama", "skill_megidola"},
        {"persona_guan_yu", "Guan Yu", "Chariot", 2, 9, 4, 5, Element::Physical, Element::Wind, "skill_cleave", "skill_slash", "skill_mighty_swing", "skill_zionga"},
        {"persona_achilles", "Achilles", "Chariot", 4, 10, 5, 9, Element::Physical, Element::Electric, "skill_slash", "skill_garula", "skill_mighty_swing", "skill_magaru"},
        {"persona_themis", "Themis", "Justice", 1, 5, 7, 5, Element::Light, Element::Dark, "skill_hama", "skill_dia", "skill_mahama", "skill_diarama"},
        {"persona_athena", "Athena", "Justice", 4, 8, 9, 7, Element::Light, Element::Dark, "skill_hama", "skill_slash", "skill_mahama", "skill_mighty_swing"},
        {"persona_heracles", "Heracles", "Strength", 2, 10, 3, 4, Element::Physical, Element::Ice, "skill_cleave", "skill_slash", "skill_mighty_swing", "skill_agilao"},
        {"persona_thor", "Thor", "Strength", 4, 11, 6, 7, Element::Electric, Element::Wind, "skill_zio", "skill_mighty_swing", "skill_zionga", "skill_mazionga"},
        {"persona_zhong_kui", "Zhong Kui", "Hermit", 3, 8, 8, 6, Element::Dark, Element::Light, "skill_mudo", "skill_slash", "skill_mamudo", "skill_mighty_swing"},
        {"persona_prometheus", "Prometheus", "Hermit", 6, 7, 15, 10, Element::Fire, Element::Ice, "skill_agilao", "skill_megidola", "skill_maragi", "skill_diarama"},
        {"persona_norn", "Norn", "Fortune", 3, 6, 10, 7, Element::Wind, Element::Electric, "skill_garula", "skill_dia", "skill_magaru", "skill_diarama"},
        {"persona_fortuna", "Fortuna", "Fortune", 6, 7, 13, 13, Element::Wind, Element::Dark, "skill_garula", "skill_megidola", "skill_magaru", "skill_mahama"},
        {"persona_odin_wanderer", "Odin Wanderer", "Hanged Man", 3, 7, 11, 6, Element::Electric, Element::Wind, "skill_zionga", "skill_dia", "skill_mazionga", "skill_megidola"},
        {"persona_loki", "Loki", "Hanged Man", 6, 7, 14, 13, Element::Dark, Element::Light, "skill_mudo", "skill_garula", "skill_mamudo", "skill_magaru"},
        {"persona_thanatos", "Thanatos", "Death", 3, 9, 10, 5, Element::Dark, Element::Light, "skill_mudo", "skill_slash", "skill_mamudo", "skill_mighty_swing"},
        {"persona_hades", "Hades", "Death", 6, 9, 17, 8, Element::Dark, Element::Light, "skill_mamudo", "skill_megidola", "skill_diarama", "skill_mighty_swing"},
        {"persona_iris", "Iris", "Temperance", 3, 5, 11, 8, Element::Wind, Element::Electric, "skill_garula", "skill_diarama", "skill_magaru", "skill_mahama"},
        {"persona_guanyin", "Guanyin", "Temperance", 6, 6, 16, 10, Element::Light, Element::Dark, "skill_mahama", "skill_diarama", "skill_megidola", "skill_dia"},
        {"persona_succubus", "Succubus", "Devil", 3, 5, 12, 8, Element::Dark, Element::Light, "skill_mudo", "skill_garula", "skill_mamudo", "skill_magaru"},
        {"persona_mara", "Mara", "Devil", 6, 12, 13, 8, Element::Dark, Element::Light, "skill_mamudo", "skill_mighty_swing", "skill_megidola", "skill_slash"},
        {"persona_minotaur", "Minotaur", "Tower", 3, 12, 5, 6, Element::Physical, Element::Light, "skill_mighty_swing", "skill_cleave", "skill_slash", "skill_agilao"},
        {"persona_typhon", "Typhon", "Tower", 6, 14, 13, 6, Element::Fire, Element::Ice, "skill_maragi", "skill_mighty_swing", "skill_megidola", "skill_agilao"},
        {"persona_astraea", "Astraea", "Star", 3, 6, 11, 8, Element::Light, Element::Dark, "skill_hama", "skill_garula", "skill_mahama", "skill_magaru"},
        {"persona_amaterasu", "Amaterasu", "Star", 6, 7, 17, 10, Element::Light, Element::Dark, "skill_mahama", "skill_maragi", "skill_megidola", "skill_diarama"},
        {"persona_artemis", "Artemis", "Moon", 3, 8, 9, 9, Element::Ice, Element::Fire, "skill_bufula", "skill_slash", "skill_mamudo", "skill_mighty_swing"},
        {"persona_tsukuyomi", "Tsukuyomi", "Moon", 6, 8, 16, 10, Element::Ice, Element::Light, "skill_bufula", "skill_mamudo", "skill_megidola", "skill_diarama"},
        {"persona_apollo", "Apollo", "Sun", 3, 7, 12, 8, Element::Fire, Element::Ice, "skill_agilao", "skill_hama", "skill_maragi", "skill_mahama"},
        {"persona_hou_yi", "Hou Yi", "Sun", 6, 15, 10, 10, Element::Fire, Element::Dark, "skill_maragi", "skill_mighty_swing", "skill_megidola", "skill_slash"},
        {"persona_anubis", "Anubis", "Judgement", 3, 7, 13, 7, Element::Dark, Element::Physical, "skill_hama", "skill_mudo", "skill_mahama", "skill_mamudo"},
        {"persona_yama", "Yama", "Judgement", 6, 10, 16, 8, Element::Dark, Element::Light, "skill_mamudo", "skill_megidola", "skill_mahama", "skill_mighty_swing"},
        {"persona_izanagi_no_okami", "Izanagi-no-Okami", "World", 5, 10, 12, 10, Element::Almighty, Element::Dark, "skill_zionga", "skill_megidola", "skill_mazionga", "skill_diarama"},
        {"persona_pangu", "Pangu", "World", 7, 16, 14, 8, Element::Physical, Element::Dark, "skill_mighty_swing", "skill_megidola", "skill_maragi", "skill_mahama"},
    }};

    personas_.clear();
    for (const auto &seed : kPersonas)
    {
        auto persona = std::make_shared<Persona>(seed.id, seed.name, seed.arcana, seed.level,
                                                 seed.strength, seed.magic, seed.speed);
        persona->setAffinity(seed.resist, Affinity::Resist);
        persona->setAffinity(seed.weak, Affinity::Weak);
        persona->learnSkill(skill(seed.skill1));
        persona->learnSkill(skill(seed.skill2));
        persona->addPotentialSkill(seed.level + 2, skill(seed.unlock1));
        persona->addPotentialSkill(seed.level + 5, skill(seed.unlock2));
        personas_.push_back(std::move(persona));
    }
}

void GameManager::initDefaultShop()
{
    shop_.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "A loaf of campus bread.", 10, 20, "items/bread"));
    shop_.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "Restores a lot of HP.", 30, 50, "items/potion_red"));
    shop_.addItem(std::make_unique<SpItem>("item_coffee", "Coffee", "Restores a little SP.", 20, 15, "items/coffee"));

    auto priceFor = [](const Persona &persona)
    {
        int statTotal = persona.strength() + persona.magic() + persona.speed();
        if (persona.level() <= 2)
            return 80 + statTotal * 8;
        if (statTotal <= 24)
            return 160 + statTotal * 12;
        return 420 + statTotal * 20;
    };
    auto arcanaTexture = [](std::string arcana)
    {
        std::transform(arcana.begin(), arcana.end(), arcana.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::replace(arcana.begin(), arcana.end(), ' ', '_');
        if (arcana == "hanged_man")
            return std::string("arcana_hanged_man");
        if (arcana == "fortune")
            return std::string("arcana_fortune");
        return std::string("arcana_") + arcana;
    };

    for (const auto &persona : personas_)
    {
        if (!persona)
            continue;
        std::string itemId = "item_" + persona->id().substr(std::string("persona_").size()) + "_contract";
        shop_.addItem(std::make_unique<PersonaItem>(itemId, persona->name() + " Contract",
                                                    "Summons " + persona->name() + " of the " + persona->arcana() + " Arcana.",
                                                    priceFor(*persona), persona->id(), arcanaTexture(persona->arcana())));
    }
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
    auto slime = std::make_unique<Slime>();
    slime->addDropPersonaId("persona_pixie");
    slime->addDropPersonaId("persona_orpheus");
    slime->addDropPersonaId("persona_kikuri_hime");
    slime->addDropPersonaId("persona_eros");
    slime->addDropPersonaId("persona_themis");
    enemyTemplates_.push_back(std::move(slime));

    auto goblin = std::make_unique<Goblin>();
    goblin->addDropPersonaId("persona_guan_yu");
    goblin->addDropPersonaId("persona_ares");
    goblin->addDropPersonaId("persona_xuanwu");
    goblin->addDropPersonaId("persona_heracles");
    goblin->addDropPersonaId("persona_norn");
    goblin->addDropPersonaId("persona_zhong_kui");
    enemyTemplates_.push_back(std::move(goblin));

    auto boss = std::make_unique<Boss>();
    boss->addDropPersonaId("persona_merlin");
    boss->addDropPersonaId("persona_athena");
    boss->addDropPersonaId("persona_thor");
    boss->addDropPersonaId("persona_achilles");
    boss->addDropPersonaId("persona_hades");
    boss->addDropPersonaId("persona_amaterasu");
    boss->addDropPersonaId("persona_yama");
    boss->addDropPersonaId("persona_pangu");
    enemyTemplates_.push_back(std::move(boss));
}

void GameManager::initDefaultSocialLinks()
{
    socialLinkManager_ = SocialLinkManager();
    generateNpcPool();
}

void GameManager::generateNpcPool()
{
    // A persistent pool of kNpcPoolSize unique NPCs per save. Names are picked
    // without replacement from a fixed English name list; portraits are chosen
    // randomly (with replacement) from the 3 available textures; arcana is
    // deterministic by index so stat-reward mapping stays stable across loads.
    static const std::vector<std::string> kNamePool = {
        "Aiden", "Bella", "Caleb", "Diana", "Ethan", "Fiona", "Gavin", "Hannah",
        "Ivan", "Julia", "Kevin", "Luna", "Mason", "Nora", "Oscar", "Piper",
        "Quinn", "Rita", "Sean", "Tina"};
    static const std::vector<std::string> kPortraits = {
        "npc_portrait_0", "npc_portrait_1", "npc_portrait_2", "npc_portrait_3"};
    static const std::vector<std::string> kSprites = {
        "npc_sprite_0", "npc_sprite_1", "npc_sprite_2", "npc_sprite_3"};
    static const std::vector<std::string> kArcanas = {
        "Magician", "Chariot", "Priestess", "Fool", "Hierophant"};

    std::mt19937 rng(std::random_device{}());

    std::vector<std::string> names = kNamePool;
    std::shuffle(names.begin(), names.end(), rng);

    npcPool_.clear();
    for (int i = 0; i < kNpcPoolSize; ++i)
    {
        NpcDefinition def;
        def.id = "sl_npc_" + std::to_string(i);
        def.name = (i < static_cast<int>(names.size())) ? names[i] : ("NPC" + std::to_string(i));
        def.portraitId = kPortraits[i % kPortraits.size()];
        def.spriteId = kSprites[i % kSprites.size()];
        def.arcana = kArcanas[i % kArcanas.size()];
        npcPool_.push_back(def);

        SocialLink link(def.id, def.name, def.arcana);
        link.setPortraitId(def.portraitId);
        socialLinkManager_.addLink(std::move(link));
    }

    applyNpcDialogueTemplates();
}

void GameManager::applyNpcDialogueTemplates()
{
    // Generic, name-agnostic dialogue shared by every pool NPC. The NPC's name
    // is prepended at fill time so the displayed text matches the loaded identity.
    static const std::vector<std::string> kLines = {
        "Oh, hey! Good to see you around.",
        "I was just thinking about our last chat.",
        "You know, I really enjoy these little talks.",
        "I feel like we're starting to click, you know?",
        "Honestly, you're one of the few people I can be myself around.",
        "Hey, whatever happens out there, I've got your back. Always.",
        "I don't say this often, but... thanks for sticking with me.",
        "You bring out a side of me I'd kind of forgotten existed.",
        "This bond we're building... it means more than you think.",
        "You're my truest friend. I hope you know that.",
        "So this is what a real bond feels like. Let's keep going, together."};

    for (SocialLink *link : socialLinkManager_.allLinks())
    {
        if (!link)
            continue;
        for (int r = 0; r <= SocialLink::kMaxRank && r < static_cast<int>(kLines.size()); ++r)
        {
            SocialLinkRankData data;
            data.dialogue = link->name() + ": \"" + kLines[r] + "\"";
            // Every rank-up grants the current Persona one level.
            data.reward.personaLevels = 1;
            link->setRankData(r, std::move(data));
        }
    }
}

void GameManager::refreshDailyNpcs()
{
    talkCountToday_.clear();
    todayNpcIds_.clear();
    todaySchoolNpcIds_.clear();

    std::vector<std::string> ids;
    ids.reserve(npcPool_.size());
    for (const auto &def : npcPool_)
        ids.push_back(def.id);

    std::mt19937 rng(std::random_device{}());
    std::shuffle(ids.begin(), ids.end(), rng);

    int totalNeeded = kTownNpcsPerDay + kSchoolNpcsPerDay;
    for (int i = 0; i < totalNeeded && i < static_cast<int>(ids.size()); ++i)
    {
        if (i < kTownNpcsPerDay)
            todayNpcIds_.push_back(ids[i]);
        else
            todaySchoolNpcIds_.push_back(ids[i]);
    }
}

void GameManager::rebuildMapNpcs()
{
    std::mt19937 rng(std::random_device{}());

    // Town map
    if (currentMap_)
    {
        TileMap &map = *currentMap_;
        engine::Vec2 playerPos = townDefaultSpawn();
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                playerPos = e->position();
        }
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));

        for (size_t i = 0; i < todayNpcIds_.size() && i < kTownNpcsPerDay; ++i)
        {
            engine::Vec2 pos = randomSpawnInZones(townNpcSpawnZones(), rng);
            const NpcDefinition *def = findNpc(todayNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i], spriteId));
        }
    }

    // School map (second map)
    if (secondMap_)
    {
        TileMap &map = *secondMap_;
        engine::Vec2 playerPos = schoolDefaultSpawn();
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                playerPos = e->position();
        }
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));

        for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kSchoolNpcsPerDay; ++i)
        {
            engine::Vec2 pos = randomSpawnInZones(schoolNpcSpawnZones(), rng);
            const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i], spriteId));
        }
    }
}

void GameManager::advanceDay()
{
    ++day_;
    refreshDailyNpcs();
    rebuildMapNpcs();
}

const GameManager::NpcDefinition *GameManager::findNpc(const std::string &id) const
{
    for (const auto &def : npcPool_)
        if (def.id == id)
            return &def;
    return nullptr;
}

int GameManager::talkCountToday(const std::string &npcId) const
{
    auto it = talkCountToday_.find(npcId);
    return it != talkCountToday_.end() ? it->second : 0;
}

std::string GameManager::talkToNpc(const std::string &socialLinkId)
{
    const int kDailyPoints = 10;

    int &count = talkCountToday_[socialLinkId];
    const SocialLink *link = socialLinkManager_.getLink(socialLinkId);

    // Hard cap: each NPC can be talked to kMaxTalksPerNpc times per day.
    if (count >= kMaxTalksPerNpc)
    {
        std::string name = link ? link->name() : "...";
        return name + " has nothing more to say today. Come back tomorrow.";
    }

    int beforeRank = link ? link->rank() : 0;
    socialLinkManager_.addPoints(socialLinkId, kDailyPoints);
    ++count;

    const SocialLink *after = socialLinkManager_.getLink(socialLinkId);
    int afterRank = after ? after->rank() : beforeRank;

    // Apply Social Link rewards: each rank gained gives current Persona levels
    // and may teach a new skill.
    Persona *persona = character_.currentPersona();
    if (persona && afterRank > beforeRank)
    {
        for (int r = beforeRank + 1; r <= afterRank; ++r)
        {
            const SocialLinkRankData *data = after->rankData(r);
            if (!data)
                continue;

            for (int i = 0; i < data->reward.personaLevels; ++i)
                persona->gainExp(persona->expToNextLevel());

            if (data->reward.newSkill)
                persona->learnSkill(data->reward.newSkill);
        }
    }

    // Fire the rank-up callback for every rank gained during this talk so
    // the UI can play the 奶龙 laugh / fire-dance sound and show a banner.
    if (rankUpCallback_ && afterRank > beforeRank)
    {
        for (int r = beforeRank + 1; r <= afterRank; ++r)
            rankUpCallback_(socialLinkId, r);
    }

    return socialLinkManager_.dialogueFor(socialLinkId);
}

void GameManager::recomputeSocialLinkBonuses()
{
    // Deprecated: Social Link rewards are now applied immediately in talkToNpc().
    // Kept as a no-op for backward compatibility with existing call sites.
}

void GameManager::initDefaultMap()
{
    currentMap_ = std::make_unique<TileMap>(25, 19);

    currentMap_->addEntity(std::make_unique<PlayerEntity>(townDefaultSpawn()));

    std::mt19937 rng(std::random_device{}());
    for (size_t i = 0; i < todayNpcIds_.size() && i < kTownNpcsPerDay; ++i)
    {
        engine::Vec2 pos = randomSpawnInZones(townNpcSpawnZones(), rng);
        const NpcDefinition *def = findNpc(todayNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        currentMap_->addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i], spriteId));
    }
}

void GameManager::initSecondMap()
{
    secondMap_ = std::make_unique<TileMap>(25, 19);

    secondMap_->addEntity(std::make_unique<PlayerEntity>(schoolDefaultSpawn()));

    std::mt19937 rng(std::random_device{}());
    for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kSchoolNpcsPerDay; ++i)
    {
        engine::Vec2 pos = randomSpawnInZones(schoolNpcSpawnZones(), rng);
        const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        secondMap_->addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i], spriteId));
    }
}

// ---- Equipment system ----

void GameManager::initDefaultEquipment()
{
    equipmentDatabase_.clear();

    // Weapon: primarily boost ATK
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_wooden", "Wooden Sword", "A simple wooden sword.", 50, 2, 0, 0, EquipmentSlot::Weapon, "tile_00_00"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_iron", "Iron Sword", "A sturdy iron blade.", 100, 5, 0, 0, EquipmentSlot::Weapon, "tile_00_01"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_steel", "Steel Sword", "A sharp steel sword.", 150, 8, 0, 0, EquipmentSlot::Weapon, "tile_00_02"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_flame", "Flame Sword", "Burns with eternal fire.", 250, 12, 0, 0, EquipmentSlot::Weapon, "tile_00_03"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_frost", "Frost Sword", "Freezes enemies on contact.", 250, 10, 2, 0, EquipmentSlot::Weapon, "tile_00_04"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_thunder", "Thunder Sword", "Crackles with lightning.", 250, 11, 0, 2, EquipmentSlot::Weapon, "tile_00_05"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_shadow", "Shadow Blade", "A blade of pure darkness.", 400, 15, 0, 3, EquipmentSlot::Weapon, "tile_00_06"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_holy", "Holy Sword", "Blessed by the gods.", 400, 14, 0, 0, EquipmentSlot::Weapon, "tile_00_07"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_dragon", "Dragon Fang", "A tooth of an ancient dragon.", 600, 18, 0, 0, EquipmentSlot::Weapon, "tile_00_08"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_breaker", "World Breaker", "Shatters reality itself.", 800, 20, 0, 0, EquipmentSlot::Weapon, "tile_00_09"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_yitian", "Yitian Sword", "A legendary blade.", 1000, 22, 0, 2, EquipmentSlot::Weapon, "tile_00_10"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_tulong", "Dragon Slayer", "Forged to slay dragons.", 1200, 25, 0, 0, EquipmentSlot::Weapon, "tile_00_11"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_judge", "Judgment", "Divine justice in blade form.", 1500, 28, 0, 0, EquipmentSlot::Weapon, "tile_00_12"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_wrath", "Wrath", "Filled with anger.", 1800, 30, 0, 0, EquipmentSlot::Weapon, "tile_00_13"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_free", "Freedom", "Unbound by fate.", 2000, 32, 0, 5, EquipmentSlot::Weapon, "tile_00_14"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("wp_dust", "Dustless", "Leaves no trace.", 2500, 35, 0, 5, EquipmentSlot::Weapon, "tile_00_15"));

    // Armor: primarily boost DEF and HP
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_cloth", "Cloth Robe", "A simple cloth robe.", 50, 0, 2, 0, EquipmentSlot::Armor, "tile_15_24"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_leather", "Leather Armor", "Tough leather protection.", 100, 0, 3, 0, EquipmentSlot::Armor, "tile_15_25"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_chain", "Chain Mail", "Interlocking metal rings.", 150, 0, 5, 0, EquipmentSlot::Armor, "tile_15_26"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_plate", "Plate Armor", "Heavy metal plates.", 200, 0, 7, 0, EquipmentSlot::Armor, "tile_16_00"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_paladin", "Paladin Armor", "Armor of the righteous.", 300, 0, 10, 0, EquipmentSlot::Armor, "tile_16_01"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_shadow", "Shadow Cloak", "Hides the wearer in darkness.", 300, 0, 8, 2, EquipmentSlot::Armor, "tile_16_02"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_dragon", "Dragon Scale", "Scales of an ancient dragon.", 400, 0, 12, 0, EquipmentSlot::Armor, "tile_16_03"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_frost", "Frost Armor", "Frozen but warm inside.", 400, 0, 11, 0, EquipmentSlot::Armor, "tile_16_04"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_flame", "Flame Armor", "Burns enemies that touch it.", 400, 2, 13, 0, EquipmentSlot::Armor, "tile_16_05"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_thunder", "Thunder Armor", "Crackles with energy.", 400, 0, 14, 3, EquipmentSlot::Armor, "tile_16_06"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_holy", "Holy Armor", "Blessed by the divine.", 500, 0, 15, 0, EquipmentSlot::Armor, "tile_16_07"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_phantom", "Phantom Cloak", "Barely visible.", 300, 0, 9, 0, EquipmentSlot::Armor, "tile_16_08"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_demon", "Demon Armor", "Forged in the abyss.", 500, 3, 16, 0, EquipmentSlot::Armor, "tile_16_11"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_angel", "Angel Robe", "Woven from light.", 500, 0, 17, 0, EquipmentSlot::Armor, "tile_16_12"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ar_war", "War God Armor", "Worn by the war god.", 800, 0, 20, 0, EquipmentSlot::Armor, "tile_16_13"));

    // Accessory: balanced stats
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_copper", "Copper Ring", "A simple copper ring.", 50, 1, 1, 1, EquipmentSlot::Accessory, "tile_08_05"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_silver", "Silver Ring", "Shines with silver light.", 100, 2, 2, 2, EquipmentSlot::Accessory, "tile_08_06"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_gold", "Gold Ring", "A symbol of wealth.", 150, 3, 3, 3, EquipmentSlot::Accessory, "tile_08_11"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_diamond", "Diamond Ring", "Sparkles brilliantly.", 200, 4, 4, 4, EquipmentSlot::Accessory, "tile_08_12"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_power", "Power Ring", "Channels raw strength.", 300, 5, 2, 0, EquipmentSlot::Accessory, "tile_08_13"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_agility", "Swift Ring", "Makes you faster.", 300, 2, 2, 5, EquipmentSlot::Accessory, "tile_08_14"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_wisdom", "Wisdom Ring", "Expands your mind.", 300, 2, 2, 0, EquipmentSlot::Accessory, "tile_08_15"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_life", "Life Ring", "Pulses with vitality.", 300, 0, 2, 0, EquipmentSlot::Accessory, "tile_08_16"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_magic", "Magic Ring", "Holds arcane power.", 300, 2, 0, 0, EquipmentSlot::Accessory, "tile_08_17"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_guard", "Guardian Ring", "Protects the wearer.", 300, 0, 5, 2, EquipmentSlot::Accessory, "tile_08_18"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_lucky", "Lucky Ring", "Brings good fortune.", 200, 3, 3, 3, EquipmentSlot::Accessory, "tile_08_19"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_brave", "Brave Ring", "Fearless in battle.", 300, 4, 3, 2, EquipmentSlot::Accessory, "tile_08_21"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_hope", "Hope Ring", "Never give up.", 300, 2, 2, 2, EquipmentSlot::Accessory, "tile_10_02"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_despair", "Despair Ring", "Crushes hope.", 300, 5, 0, 0, EquipmentSlot::Accessory, "tile_10_04"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_chaos", "Chaos Ring", "Unpredictable power.", 400, 3, 3, 3, EquipmentSlot::Accessory, "tile_10_16"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_order", "Order Ring", "Brings balance.", 400, 3, 3, 3, EquipmentSlot::Accessory, "tile_10_17"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("ac_fate", "Fate Ring", "Controls destiny.", 500, 5, 5, 5, EquipmentSlot::Accessory, "tile_10_18"));

    // Relic: balanced stats (stronger)
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_ancient", "Ancient Scroll", "Knowledge of the past.", 300, 3, 3, 2, EquipmentSlot::Relic, "tile_03_26"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_crystal", "Mystic Crystal", "Holds unknown power.", 400, 4, 4, 3, EquipmentSlot::Relic, "tile_04_00"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_soul", "Soul Stone", "Contains a trapped soul.", 500, 5, 5, 4, EquipmentSlot::Relic, "tile_04_01"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_life", "Life Source", "Origin of all life.", 600, 3, 5, 3, EquipmentSlot::Relic, "tile_04_02"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_wisdom", "Wisdom Light", "Illuminates the truth.", 600, 4, 3, 3, EquipmentSlot::Relic, "tile_04_03"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_power", "Power Core", "Raw strength condensed.", 600, 8, 3, 3, EquipmentSlot::Relic, "tile_04_20"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_speed", "Speed Wind", "Faster than the wind.", 600, 3, 3, 8, EquipmentSlot::Relic, "tile_04_25"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_defense", "Defense Shield", "Impenetrable barrier.", 600, 3, 8, 3, EquipmentSlot::Relic, "tile_05_00"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_attack", "Attack Blade", "Cuts through anything.", 600, 8, 3, 3, EquipmentSlot::Relic, "tile_05_04"));
    equipmentDatabase_.push_back(std::make_shared<EquipmentItem>("rl_magic", "Magic Staff", "Channels all magic.", 800, 5, 5, 5, EquipmentSlot::Relic, "tile_05_05"));
}

void GameManager::equipItem(std::shared_ptr<EquipmentItem> item)
{
    if (!item)
        return;

    // Unequip current item in the same slot.
    unequipItem(item->slot());

    // Equip the new item.
    switch (item->slot())
    {
    case EquipmentSlot::Weapon:
        equippedGear_.weapon = item;
        break;
    case EquipmentSlot::Armor:
        equippedGear_.armor = item;
        break;
    case EquipmentSlot::Accessory:
        equippedGear_.accessory = item;
        break;
    case EquipmentSlot::Relic:
        equippedGear_.relic = item;
        break;
    default:
        return;
    }

    // Apply equipment bonuses.
    character_.equip(item);
}

void GameManager::unequipItem(EquipmentSlot slot)
{
    std::shared_ptr<EquipmentItem> item;
    switch (slot)
    {
    case EquipmentSlot::Weapon:
        item = equippedGear_.weapon;
        equippedGear_.weapon = nullptr;
        break;
    case EquipmentSlot::Armor:
        item = equippedGear_.armor;
        equippedGear_.armor = nullptr;
        break;
    case EquipmentSlot::Accessory:
        item = equippedGear_.accessory;
        equippedGear_.accessory = nullptr;
        break;
    case EquipmentSlot::Relic:
        item = equippedGear_.relic;
        equippedGear_.relic = nullptr;
        break;
    default:
        return;
    }

    if (!item)
        return;

    // Remove equipment bonuses.
    character_.setEquipmentBonuses(
        character_.equipmentStrengthBonus() - item->strengthBonus(),
        character_.equipmentMagicBonus() - item->magicBonus(),
        character_.equipmentSpeedBonus() - item->speedBonus());

}

bool GameManager::equipFromInventory(size_t index)
{
    Item *raw = inventory_.itemAt(index);
    if (!raw)
        return false;
    auto *eq = dynamic_cast<EquipmentItem *>(raw);
    if (!eq || eq->slot() == EquipmentSlot::None)
        return false;

    EquipmentSlot slot = eq->slot();

    // Detach the item from the inventory first so the old equipped item has a
    // place to land when unequipItem puts it back.
    auto taken = inventory_.takeItem(index);
    if (!taken)
        return false;

    // Convert the detached unique_ptr into the shared_ptr the gear slot holds.
    auto shared = std::shared_ptr<EquipmentItem>(
        static_cast<EquipmentItem *>(taken.release()));

    // Unequip whatever was in that slot and put it back into the inventory.
    std::shared_ptr<EquipmentItem> oldItem;
    switch (slot)
    {
    case EquipmentSlot::Weapon:
        oldItem = equippedGear_.weapon;
        break;
    case EquipmentSlot::Armor:
        oldItem = equippedGear_.armor;
        break;
    case EquipmentSlot::Accessory:
        oldItem = equippedGear_.accessory;
        break;
    case EquipmentSlot::Relic:
        oldItem = equippedGear_.relic;
        break;
    default:
        return false;
    }
    unequipItem(slot);
    if (oldItem)
        inventory_.addItem(oldItem->clone());

    // Place the new item into the slot and apply its bonuses.
    switch (slot)
    {
    case EquipmentSlot::Weapon:
        equippedGear_.weapon = shared;
        break;
    case EquipmentSlot::Armor:
        equippedGear_.armor = shared;
        break;
    case EquipmentSlot::Accessory:
        equippedGear_.accessory = shared;
        break;
    case EquipmentSlot::Relic:
        equippedGear_.relic = shared;
        break;
    default:
        return false;
    }
    character_.equip(shared);
    return true;
}
