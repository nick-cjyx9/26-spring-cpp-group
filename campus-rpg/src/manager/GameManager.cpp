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
#include "PauseMenuScene.h"
#include "QuestScene.h"

#include <algorithm>
#include <array>
#include <cmath>
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

    float distance(const engine::Vec2 &a, const engine::Vec2 &b)
    {
        float dx = a.x - b.x;
        float dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    // Pick a random spawn position that is at least minDist away from every
    // position in existing. Falls back to the first valid attempt if no
    // fully-satisfying spot is found within maxRetries.
    engine::Vec2 randomSpawnInZones(const std::vector<engine::Rect> &zones, std::mt19937 &rng,
                                    const std::vector<engine::Vec2> &existing, float minDist)
    {
        constexpr int kMaxRetries = 50;
        engine::Vec2 bestPos = randomSpawnInZones(zones, rng);
        for (int attempt = 0; attempt < kMaxRetries; ++attempt)
        {
            engine::Vec2 pos = randomSpawnInZones(zones, rng);
            bool ok = true;
            for (const auto &p : existing)
            {
                if (distance(pos, p) < minDist)
                {
                    ok = false;
                    break;
                }
            }
            if (ok)
                return pos;
            if (attempt == 0)
                bestPos = pos;
        }
        return bestPos;
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
    case SceneType::PauseMenu:
        currentScene_ = std::make_unique<PauseMenuScene>();
        break;
    case SceneType::Quest:
        currentScene_ = std::make_unique<QuestScene>();
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
    shop_.addItem(std::make_unique<EquipmentItem>("eq_wooden_sword", "Wooden Sword",
                                                  "A training sword.", 80, 5, 0, 0,
                                                  EquipmentSlot::Weapon, "tile_00_00"));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_leather_armor", "Leather Armor",
                                                  "Basic protection.", 60, 0, 4, 0,
                                                  EquipmentSlot::Armor, "tile_15_24"));

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
    // Helper lambda to add kill quests
    auto addKillQuest = [this](const std::string &id, const std::string &name,
                                const std::string &desc, const std::string &condition,
                                int gold, int exp, const std::string &npcId)
    {
        Quest q(id, name, desc, condition, gold, exp);
        q.setType(QuestType::Kill);
        q.setNpcId(npcId);
        questManager_.addQuest(std::move(q));
    };

    // Helper lambda to add collect quests
    auto addCollectQuest = [this](const std::string &id, const std::string &name,
                                   const std::string &desc, const std::string &itemId,
                                   int count, int gold, int exp, const std::string &npcId)
    {
        Quest q(id, name, desc, "", gold, exp);
        q.setType(QuestType::Collect);
        q.setNpcId(npcId);
        q.setTargetItemId(itemId);
        q.setTargetCount(count);
        questManager_.addQuest(std::move(q));
    };

    // Zhou (Fool) - Yellow Braised Chicken Shop Owner
    addCollectQuest("quest_zhou_1", "The Secret of the Golden Casserole", "Help Zhou collect 3 Shadow Mushrooms.",
                    "item_shadow_mushroom", 3, 300, 50, "sl_npc_0");
    addKillQuest("quest_zhou_2", "The Taste of Protection", "Defeat 5 Slimes to protect the ingredient delivery route.",
                 "kill:5", 500, 80, "sl_npc_0");

    // Eric (Magician) - Alchemist
    addCollectQuest("quest_eric_1", "Alchemy Materials", "Help Eric collect 2 Crystal Shards.",
                    "item_crystal_shard", 2, 200, 40, "sl_npc_1");
    addKillQuest("quest_eric_2", "Monster Research", "Defeat 3 Goblins and collect their tissue samples.",
                 "kill:3", 400, 60, "sl_npc_1");

    // Selena (High Priestess) - Prophet
    addCollectQuest("quest_selena_1", "Starlight Herbs", "Help Selena collect 3 Starlight Herbs.",
                    "item_star_herb", 3, 250, 45, "sl_npc_2");
    addKillQuest("quest_selena_2", "Threat of Shadows", "Defeat 2 Shadow monsters.",
                 "kill:2", 350, 55, "sl_npc_2");

    // Maria (Empress) - Restaurant Owner
    addCollectQuest("quest_maria_1", "Fresh Wheat", "Help Maria collect 5 portions of Flour.",
                    "item_flour", 5, 150, 30, "sl_npc_3");
    addKillQuest("quest_maria_2", "Escort Mission", "Defeat 1 Boss to protect the bread delivery route.",
                 "kill:1", 600, 100, "sl_npc_3");

    // Arthur (Emperor) - Town Guard Captain
    addKillQuest("quest_arthur_1", "Recruit Trial", "Defeat 3 Slimes.",
                 "kill:3", 300, 50, "sl_npc_4");
    addKillQuest("quest_arthur_2", "Source of Shadows", "Defeat 5 Goblins.",
                 "kill:5", 500, 70, "sl_npc_4");

    // Thomas (Hierophant) - History Professor
    addCollectQuest("quest_thomas_1", "Ancient Book Collection", "Help Thomas collect 3 Ancient Books.",
                    "item_book", 3, 200, 35, "sl_npc_5");
    addKillQuest("quest_thomas_2", "The Price of Knowledge", "Defeat 4 Shadow monsters.",
                 "kill:4", 450, 65, "sl_npc_5");

    // Maxim (Chariot) - Hot-blooded Warrior
    addKillQuest("quest_maxim_1", "Pre-Battle Warm-up", "Defeat 5 Slimes.",
                 "kill:5", 250, 40, "sl_npc_6");
    addKillQuest("quest_maxim_2", "Endless Challenge", "Defeat 10 Goblins.",
                 "kill:10", 800, 120, "sl_npc_6");

    // Reina (Strength) - Beast Tamer
    addCollectQuest("quest_reina_1", "Healing Herbs", "Help Reina collect 3 Healing Herbs.",
                    "item_herb", 3, 200, 40, "sl_npc_7");
    addKillQuest("quest_reina_2", "Protect the Monsters", "Defeat 2 Shadow monsters.",
                 "kill:2", 350, 55, "sl_npc_7");

    // Zhang (Hermit) - Mysterious Old Man
    addCollectQuest("quest_zhang_1", "Key Fragments", "Help Zhang collect 3 Key Fragments.",
                    "item_key_fragment", 3, 500, 80, "sl_npc_8");
    addKillQuest("quest_zhang_2", "The Final Test", "Defeat 1 Boss.",
                 "kill:1", 1000, 150, "sl_npc_8");

    // Lily (Lovers) - Dual-faced Girl
    addCollectQuest("quest_lily_1", "Secret Garden", "Help Lily collect 5 Glowing Flowers.",
                    "item_flower", 5, 100, 25, "sl_npc_9");
    addKillQuest("quest_lily_2", "Protect Lily", "Defeat 3 Slimes.",
                 "kill:3", 250, 40, "sl_npc_9");
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
    // Fixed pool of 10 tarot-themed NPCs with English names.
    static const std::vector<NpcDefinition> kFixedPool = {
        {"sl_npc_0", "Zhou", "npc_portrait_0", "npc_sprite_0", "Fool"},
        {"sl_npc_1", "Eric", "npc_portrait_1", "npc_sprite_1", "Magician"},
        {"sl_npc_2", "Selena", "npc_portrait_2", "npc_sprite_2", "High Priestess"},
        {"sl_npc_3", "Maria", "npc_portrait_3", "npc_sprite_3", "Empress"},
        {"sl_npc_4", "Arthur", "npc_portrait_4", "npc_sprite_4", "Emperor"},
        {"sl_npc_5", "Thomas", "npc_portrait_5", "npc_sprite_5", "Hierophant"},
        {"sl_npc_6", "Maxim", "npc_portrait_6", "npc_sprite_6", "Chariot"},
        {"sl_npc_7", "Reina", "npc_portrait_7", "npc_sprite_7", "Strength"},
        {"sl_npc_8", "Zhang", "npc_portrait_8", "npc_sprite_8", "Hermit"},
        {"sl_npc_9", "Lily", "npc_portrait_9", "npc_sprite_9", "Lovers"}};

    npcPool_.clear();
    for (int i = 0; i < kNpcPoolSize && i < static_cast<int>(kFixedPool.size()); ++i)
    {
        npcPool_.push_back(kFixedPool[i]);
        SocialLink link(kFixedPool[i].id, kFixedPool[i].name, kFixedPool[i].arcana);
        link.setPortraitId(kFixedPool[i].portraitId);
        socialLinkManager_.addLink(std::move(link));
    }

    applyNpcDialogueTemplates();
}

void GameManager::applyNpcDialogueTemplates()
{
    // Per-NPC unique dialogues keyed by npc id.
    // Each NPC has 11 entries (rank 0..10). Each entry is a vector of 3 dialogue lines.
    static const std::map<std::string, std::vector<std::vector<std::string>>> kNpcDialogues = {
        // Zhou - Fool: Yellow Braised Chicken Shop Owner
        {"sl_npc_0",
         {
             {"Zhou: \"Welcome! The usual? A big bowl of Yellow Braised Chicken?\"", "Zhou: \"The secret recipe for this Yellow Braised Chicken came all the way from my hometown.\"", "Zhou: \"In this world, being able to taste the flavor of home is a real blessing.\""},
             {"Zhou: \"I've been running low on ingredients lately. Could you grab me some fresh mushrooms?\"", "Zhou: \"You gotta pick the ones glistening with morning dew. That's how you get the real freshness.\"", "Zhou: \"With you around, this place finally feels alive again.\""},
             {"Zhou: \"Heard the monsters at night drop some strange ingredients?\"", "Zhou: \"If you could get me those glowing mushrooms, they'd make for a killer dish.\"", "Zhou: \"You've got guts, kid, but stay safe out there.\""},
             {"Zhou: \"Tried cooking something new yesterday. Wanna give it a taste?\"", "Zhou: \"It's not quite Yellow Braised Chicken, but it's got its own charm.\"", "Zhou: \"Feedback is important, so help me improve it.\""},
             {"Zhou: \"Ever wonder why it was you who ended up in this world?\"", "Zhou: \"Maybe Yellow Braised Chicken is the bridge connecting our two worlds.\"", "Zhou: \"Anyway, since you're here, make the most of it.\""},
             {"Zhou: \"The seasonings here aren't quite like back home, but they've got their own flavor.\"", "Zhou: \"I'm doing my best to recreate that authentic taste.\"", "Zhou: \"Whenever you feel homesick, come have a bowl of Yellow Braised Chicken.\""},
             {"Zhou: \"Had some strange customers lately. They'd eat and just stare into space.\"", "Zhou: \"They say my Yellow Braised Chicken makes them remember the past.\"", "Zhou: \"Guess my cooking really is magical, huh?\""},
             {"Zhou: \"When I was young, I wanted to be a hero too. Then I chose cooking.\"", "Zhou: \"Everyone's got their own battlefield. Mine is the kitchen.\"", "Zhou: \"Making food that brings people happiness is a kind of heroism too.\""},
             {"Zhou: \"That tarot aura around you is getting stronger.\"", "Zhou: \"Personas... those are mysterious things.\"", "Zhou: \"But no matter what, remember to come back for a meal.\""},
             {"Zhou: \"Been thinking about opening a branch near the school.\"", "Zhou: \"That way you could grab a bite on your way to and from school.\"", "Zhou: \"Haha, just kidding. Let's focus on keeping this place running first.\""},
             {"Zhou: \"You're quite the celebrity around town now. Everyone says you're a hero.\"", "Zhou: \"But to me, you'll always be that kid who loves Yellow Braised Chicken.\"", "Zhou: \"No matter what happens, this place will always be your home.\""},
         }},
        // Eric - Magician: Alchemist
        {"sl_npc_1",
         {
             {"Eric: \"Welcome to my lab. Careful with the bottles and jars.\"", "Eric: \"I'm studying the alchemy of this world. Fascinating stuff.\"", "Eric: \"Interested in alchemy? I could teach you a thing or two.\""},
             {"Eric: \"Lately I've been trying to replicate the flavor of Yellow Braised Chicken, but something's always missing.\"", "Eric: \"Maybe it needs a magical catalyst?\"", "Eric: \"Or perhaps... a tear from another world?\""},
             {"Eric: \"See this potion? It can temporarily boost your Persona.\"", "Eric: \"But the side effect is... temporary memory loss.\"", "Eric: \"Just kidding. At worst, you'll have some gas.\""},
             {"Eric: \"I found an ancient alchemical formula in an old book.\"", "Eric: \"Supposedly it can summon a legendary Persona.\"", "Eric: \"But it requires 22 materials. Want to help me gather them?\""},
             {"Eric: \"Notice those strange runes on the night monsters?\"", "Eric: \"I suspect they're some kind of alchemical creation.\"", "Eric: \"If we could decode those runes, we might create even more powerful Personas.\""},
             {"Eric: \"Impatience is the enemy of good experiments. Alchemy requires patience.\"", "Eric: \"Just like Yellow Braised Chicken needs to simmer slowly.\"", "Eric: \"...Wait, why am I thinking about Yellow Braised Chicken?\""},
             {"Eric: \"I discovered a new catalyst that can evolve a Persona.\"", "Eric: \"But there's only a 50% success rate. Feeling lucky?\"", "Eric: \"Don't worry. At worst, your Persona just drops a level.\""},
             {"Eric: \"I'm running low on research funds. Could you help me gather some materials to sell?\"", "Eric: \"Those glowing mushrooms fetch a high price on the black market.\"", "Eric: \"Don't worry, I'll cut you in on the profits.\""},
             {"Eric: \"I've been studying the relationships between the 22 tarot cards.\"", "Eric: \"There's a fascinating resonance between them.\"", "Eric: \"If we could make all Personas resonate... maybe we could open the door home.\""},
             {"Eric: \"My latest invention: the portable Persona storage device!\"", "Eric: \"It can only store one Persona, but it's still convenient.\"", "Eric: \"...Okay, it's actually just a box. Forget I said anything.\""},
             {"Eric: \"Since I met you, my experiments have made breakthrough progress.\"", "Eric: \"Maybe friendship itself is the greatest alchemy of all.\"", "Eric: \"Come on, let me treat you to Yellow Braised Chicken tonight... if Zhou approves.\""},
         }},
        // Selena - High Priestess: Prophet
        {"sl_npc_2",
         {
             {"Selena: \"I saw your silhouette in the crystal ball.\"", "Selena: \"Surrounded by golden light, you fell from the sky.\"", "Selena: \"Your fate is deeply intertwined with the tarot of this world.\""},
             {"Selena: \"The stars last night revealed that your Persona is about to awaken.\"", "Selena: \"Are you ready to embrace this new power?\"", "Selena: \"Remember, the greater the power, the greater the responsibility.\""},
             {"Selena: \"I sense an unusual energy fluctuation in the northern forest.\"", "Selena: \"A rare Persona may be hidden there.\"", "Selena: \"But be cautious... shadows also lurk in those woods.\""},
             {"Selena: \"A strange vision appeared in the crystal ball... a bowl of golden chicken?\"", "Selena: \"That is your past, and the source of your strength.\"", "Selena: \"Never forget who you are.\""},
             {"Selena: \"The moon is especially full tonight, perfect for a divination ritual.\"", "Selena: \"Would you like a glimpse of the future?\"", "Selena: \"...No, some paths you must walk alone.\""},
             {"Selena: \"I feel the confusion and homesickness in your heart.\"", "Selena: \"But believe that every journey has its meaning.\"", "Selena: \"When you gather all 22 tarot cards, the answers will reveal themselves.\""},
             {"Selena: \"The prophecy foretells a powerful enemy appearing on the night of the full moon.\"", "Selena: \"It is your trial, and an opportunity for growth.\"", "Selena: \"Prepare yourself, but do not fear.\""},
             {"Selena: \"Do you know? Each tarot card holds the potential of a Persona.\"", "Selena: \"As your bonds with others deepen, the corresponding mask awakens.\"", "Selena: \"Go and speak with the people of this town.\""},
             {"Selena: \"I see two paths diverging before you.\"", "Selena: \"One leads home, the other to deeper bonds.\"", "Selena: \"Whichever you choose, it will not be wrong.\""},
             {"Selena: \"My visions have grown clouded of late.\"", "Selena: \"Something seems to be interfering with the threads of fate.\"", "Selena: \"Perhaps it is time to investigate.\""},
             {"Selena: \"You have finally reached this point.\"", "Selena: \"All 22 tarot cards are gathered. The path home lies before you.\"", "Selena: \"But remember, whether you stay or leave, there will always be a place for you here.\""},
         }},
        // Maria - Empress: Restaurant Owner
        {"sl_npc_3",
         {
             {"Maria: \"Welcome! What can I get you?\"", "Maria: \"Zhou's Yellow Braised Chicken is the specialty, but my bread is pretty amazing too.\"", "Maria: \"Don't be shy, make yourself at home.\""},
             {"Maria: \"Flour prices have gone up lately. I can barely afford to bake bread anymore.\"", "Maria: \"Could you help me find a cheaper source of ingredients?\"", "Maria: \"Of course, I'll repay you with delicious bread.\""},
             {"Maria: \"Zhou is obsessed with his Yellow Braised Chicken and ignores the rest of the menu.\"", "Maria: \"I want to try some new dishes, but I need a taste tester.\"", "Maria: \"Would you be my taste tester?\""},
             {"Maria: \"I heard there's a type of magical honey in the forest behind the school.\"", "Maria: \"Bread made with that honey would be absolutely divine.\"", "Maria: \"But those bees seem... a bit aggressive?\""},
             {"Maria: \"You've really been looking like a hero lately.\"", "Maria: \"But don't forget, even heroes need to eat.\"", "Maria: \"Here, fresh out of the oven. Eat it while it's hot.\""},
             {"Maria: \"I'm thinking of opening a dessert shop, selling cakes and cookies.\"", "Maria: \"What do you think of the idea?\"", "Maria: \"If it works out, you can have free cake every day.\""},
             {"Maria: \"Zhou says you miss home.\"", "Maria: \"I don't know what 'Yellow Braised Chicken' is, but I can feel your longing.\"", "Maria: \"Here, have some bread. Consider this place your home.\""},
             {"Maria: \"A mysterious guest gave me a strange key last night.\"", "Maria: \"They said it opens some treasure chest, but I don't know where it is.\"", "Maria: \"Interested in helping me find it?\""},
             {"Maria: \"I found that using materials dropped by monsters in bread gives special effects!\"", "Maria: \"Like temporarily boosting your attack power.\"", "Maria: \"...Of course, I'll make sure it's safe. Probably.\""},
             {"Maria: \"You adventurers are always in such a rush.\"", "Maria: \"But remember, no matter how far you go, come back for a meal.\"", "Maria: \"A warm meal is the best medicine for the soul.\""},
             {"Maria: \"Watching you go from a confused newcomer to a hero has filled my heart with joy.\"", "Maria: \"It's like watching my own child grow up.\"", "Maria: \"Whatever you choose, Big Sis Maria will always support you.\""},
         }},
        // Arthur - Emperor: Town Guard Captain
        {"sl_npc_4",
         {
             {"Arthur: \"Halt! Oh, it's you.\"", "Arthur: \"More monsters have been appearing around town. We need to increase patrols.\"", "Arthur: \"If you have time, help us clear them out.\""},
             {"Arthur: \"Monsters breached the eastern wall again last night.\"", "Arthur: \"They were repelled quickly, but it's a bad sign.\"", "Arthur: \"Can you investigate why the monsters are getting stronger?\""},
             {"Arthur: \"I hear you've made quite a few friends lately.\"", "Arthur: \"Good. In this world, allies are strength.\"", "Arthur: \"But remember, true strength comes from within.\""},
             {"Arthur: \"I'm training new recruits, but they lack combat experience.\"", "Arthur: \"Could you take a few out into the field?\"", "Arthur: \"Don't worry, you'll be well compensated.\""},
             {"Arthur: \"There's a rumor of a powerful boss in the northern forest.\"", "Arthur: \"Taking it down would greatly improve the town's safety.\"", "Arthur: \"...Of course, know your limits. Don't overdo it.\""},
             {"Arthur: \"Do you know why this town still stands in such a dangerous world?\"", "Arthur: \"Because we stand united.\"", "Arthur: \"You are part of this family too. Never forget that.\""},
             {"Arthur: \"There have been reports of suspicious figures at night.\"", "Arthur: \"It could be a new threat, or just a lost traveler.\"", "Arthur: \"Could you check it out?\""},
             {"Arthur: \"I was a hot-blooded adventurer myself when I was young.\"", "Arthur: \"But I realized that protecting home is more important than adventuring.\"", "Arthur: \"What you're doing now is what I once dreamed of doing.\""},
             {"Arthur: \"The town's granary is almost empty. We need supplies urgently.\"", "Arthur: \"Can you hunt some monsters and bring back meat?\"", "Arthur: \"Everyone needs to eat.\""},
             {"Arthur: \"There are rumors of a 23rd tarot card being spotted.\"", "Arthur: \"I don't know if it's true, but if it is...\"", "Arthur: \"It might change the fate of this world.\""},
             {"Arthur: \"You have become this town's hero.\"", "Arthur: \"But being a hero is not the end, it's the beginning.\"", "Arthur: \"More challenges await. Be ready, young hero.\""},
         }},
        // Thomas - Hierophant: History Professor
        {"sl_npc_5",
         {
             {"Thomas: \"Welcome to the halls of knowledge.\"", "Thomas: \"I've been studying the history of this world and found many fascinating secrets.\"", "Thomas: \"Are you interested in history?\""},
             {"Thomas: \"According to ancient texts, this world was once at peace.\"", "Thomas: \"Until one day, 22 beams of light fell from the sky, bringing forth Personas.\"", "Thomas: \"But with them came the shadow monsters.\""},
             {"Thomas: \"I found records of 'visitors from another world' in an ancient tome.\"", "Thomas: \"Apparently, every few centuries, someone crosses over from another world.\"", "Thomas: \"And you... seem to be the chosen one this time.\""},
             {"Thomas: \"Some strange books have appeared in the school library lately.\"", "Thomas: \"They appeared on their own, filled with deep secrets of the tarot.\"", "Thomas: \"Would you help me organize them?\""},
             {"Thomas: \"Did you know? Each tarot card represents the pinnacle of a personality.\"", "Thomas: \"The Fool represents new beginnings, the Magician creation, the High Priestess wisdom...\"", "Thomas: \"And you... seem to carry the potential of all of them.\""},
             {"Thomas: \"I'm studying an ancient ritual.\"", "Thomas: \"Supposedly, it can summon the legendary 'World' Persona.\"", "Thomas: \"But it requires the power of all 22 tarot cards. Interested?\""},
             {"Thomas: \"Some students claim to have seen ghosts at night.\"", "Thomas: \"I believe it might be residual Persona energy.\"", "Thomas: \"Could you investigate? You might find a new Persona.\""},
             {"Thomas: \"I've been organizing notes on Persona fusion.\"", "Thomas: \"When two Personas of different tarot cards fuse, unexpected effects occur.\"", "Thomas: \"If you have extra Personas, come give it a try.\""},
             {"Thomas: \"Ancient texts mention that when the light of all 22 tarot cards converges, the 'Gate of Truth' appears.\"", "Thomas: \"That gate leads to the truth of the world, and perhaps to your homeland.\"", "Thomas: \"But be careful, truth often comes at a price.\""},
             {"Thomas: \"I have spent my life pursuing knowledge.\"", "Thomas: \"But lately I've begun to wonder if some knowledge should remain unknown.\"", "Thomas: \"If it were you, would you choose to know the truth, or remain in blissful ignorance?\""},
             {"Thomas: \"At last, you have gathered all the tarot cards.\"", "Thomas: \"Now, you may choose to open the 'Gate of Truth' and return home.\"", "Thomas: \"Or stay in this world and become a new legend. Whatever you choose, history will remember your name.\""},
         }},
        // Maxim - Chariot: Hot-blooded Warrior
        {"sl_npc_6",
         {
             {"Maxim: \"Hey! Perfect timing, spar with me!\"", "Maxim: \"I've been itching for a fight but can't find anyone.\"", "Maxim: \"Don't worry, I'll go easy on you! Probably...\""},
             {"Maxim: \"You know what? I think Yellow Braised Chicken boosts combat power!\"", "Maxim: \"Every time I eat Zhou's Yellow Braised Chicken, I feel my Persona getting stronger.\"", "Maxim: \"...Okay, maybe it's just in my head.\""},
             {"Maxim: \"Went night hunting yesterday and fought an insanely strong monster!\"", "Maxim: \"Barely made it back alive, but that thrill of battle... incredible!\"", "Maxim: \"You should come too! I got your back!\""},
             {"Maxim: \"I'm searching for the legendary strongest Persona.\"", "Maxim: \"They say it hides in the deepest shadows.\"", "Maxim: \"When I find it, I'm definitely fighting you!\""},
             {"Maxim: \"Honestly, I envy you.\"", "Maxim: \"You come from a peaceful world with no monsters, no battles.\"", "Maxim: \"But maybe that's exactly why you were sent to a place that needs you.\""},
             {"Maxim: \"I developed a new move recently!\"", "Maxim: \"It's called 'Yellow Braised Chicken Cyclone Slash'!\"", "Maxim: \"...Okay, it actually has nothing to do with Zhou's dish. I just thought it sounded cool.\""},
             {"Maxim: \"Heard there are ancient warrior relics in the northern ruins.\"", "Maxim: \"If we found them, Persona power would increase dramatically.\"", "Maxim: \"Let's go explore together!\""},
             {"Maxim: \"I realized that in battle, the deeper the bond between Persona and person, the stronger the power.\"", "Maxim: \"So I've decided we should be the best partners!\"", "Maxim: \"Come on, let's fight to build that bond!\""},
             {"Maxim: \"Lately I've been feeling unmotivated...\"", "Maxim: \"Probably because I haven't had Yellow Braised Chicken in a while.\"", "Maxim: \"Let's go grab a meal at Zhou's, my treat!\""},
             {"Maxim: \"I've been thinking about what true strength really is.\"", "Maxim: \"Is it defeating all enemies? Or protecting those who matter?\"", "Maxim: \"Now I get it. True strength is doing both.\""},
             {"Maxim: \"You've become the strongest warrior.\"", "Maxim: \"But I know your journey isn't over yet.\"", "Maxim: \"Let's go, to the final battlefield! I'll be right by your side!\""},
         }},
        // Reina - Strength: Beast Tamer
        {"sl_npc_7",
         {
             {"Reina: \"Shh... keep it down, you'll scare them.\"", "Reina: \"These monsters aren't really scary, just misunderstood.\"", "Reina: \"Want to pet one? They're very gentle.\""},
             {"Reina: \"I'm studying the relationship between monsters and Personas.\"", "Reina: \"Turns out they're different expressions of the same energy.\"", "Reina: \"If we could communicate with monsters, we might avoid many battles.\""},
             {"Reina: \"The forest monsters have been agitated lately.\"", "Reina: \"I sense they're afraid of something.\"", "Reina: \"Could you investigate? Remember, try not to hurt them.\""},
             {"Reina: \"I found a way to form bonds with monsters.\"", "Reina: \"Just like forming bonds with Personas.\"", "Reina: \"They won't become partners, but at least they won't attack you.\""},
             {"Reina: \"Did you know? Some monsters actually love Yellow Braised Chicken too.\"", "Reina: \"Last time Zhou dropped a piece, a little monster snatched it up.\"", "Reina: \"Watching it eat so happily made me hungry too.\""},
             {"Reina: \"I'm caring for an injured monster cub.\"", "Reina: \"It needs a special herb to heal.\"", "Reina: \"Could you help me find it? It should be deep in the forest.\""},
             {"Reina: \"Some people have been killing monsters just to collect materials.\"", "Reina: \"It makes me so sad...\"", "Reina: \"Could you help stop this? At least, don't become one of them.\""},
             {"Reina: \"I discovered monsters have attributes similar to tarot cards.\"", "Reina: \"Some correspond to 'Strength', others to 'Justice'.\"", "Reina: \"Understanding their attributes would make battles easier.\""},
             {"Reina: \"Last night I dreamed of a world where monsters and humans lived in peace.\"", "Reina: \"I know it was just a dream, but maybe...\"", "Reina: \"Maybe one day it can come true.\""},
             {"Reina: \"Someone gave me a monster egg.\"", "Reina: \"I want to hatch it, but I don't know how to care for it.\"", "Reina: \"Could you help me gather knowledge on raising young creatures?\""},
             {"Reina: \"Thank you for always supporting me.\"", "Reina: \"Because of you, I believe bonds between humans and monsters are truly possible.\"", "Reina: \"No matter what the future holds, I will continue to protect this belief.\""},
         }},
        // Zhang - Hermit: Mysterious Old Man
        {"sl_npc_8",
         {
             {"Zhang: \"You're here? Sit.\"", "Zhang: \"I know you came from another world.\"", "Zhang: \"Only someone from another world would be so obsessed with Yellow Braised Chicken.\""},
             {"Zhang: \"This world has 22 Personas, corresponding to 22 tarot cards.\"", "Zhang: \"But within you, there seems to be a 23rd possibility.\"", "Zhang: \"That might just be... the key to returning home.\""},
             {"Zhang: \"Last night I observed the stars and found your destiny star shone exceptionally bright.\"", "Zhang: \"But it was also shrouded in shadow.\"", "Zhang: \"Be careful. The greatest enemy often comes from within.\""},
             {"Zhang: \"I discovered an ancient ruin in the mountains.\"", "Zhang: \"It contains secrets about 'travelers from other worlds'.\"", "Zhang: \"But the guardian there is powerful. You must prepare.\""},
             {"Zhang: \"Have you ever wondered why it was you who crossed into this world?\"", "Zhang: \"Perhaps it was due to some intense longing.\"", "Zhang: \"And your longing for Yellow Braised Chicken is the bond connecting you to this world.\""},
             {"Zhang: \"In my youth I was also an adventurer, and dreamed of crossing to other worlds.\"", "Zhang: \"But I never imagined the one who would come would be you, from another world.\"", "Zhang: \"Fate is truly mysterious.\""},
             {"Zhang: \"Recently I sensed a powerful shadow force gathering.\"", "Zhang: \"If it fully awakens, the entire town will be destroyed.\"", "Zhang: \"Can you stop it? Before it's too late.\""},
             {"Zhang: \"I found a ritual in an ancient text.\"", "Zhang: \"It can temporarily open a passage to another world.\"", "Zhang: \"But it requires immense energy, and the gate will only remain open briefly.\""},
             {"Zhang: \"You've gathered many tarot cards, but remember: power is a means, not an end.\"", "Zhang: \"What truly matters are the bonds and memories you've made along the way.\"", "Zhang: \"Those are your most precious treasures.\""},
             {"Zhang: \"The final trial approaches.\"", "Zhang: \"Whether you choose to stay or leave, you must face the shadow within your own heart.\"", "Zhang: \"Remember, true strength is not defeating enemies, but conquering yourself.\""},
             {"Zhang: \"You have finally reached this point.\"", "Zhang: \"All 22 tarot cards are gathered. The path home lies before you.\"", "Zhang: \"But before that, one last question: are you truly ready?\""},
         }},
        // Lily - Lovers: Dual-faced Girl
        {"sl_npc_9",
         {
             {"Lily: \"Hey! You're here!\"", "Lily: \"The weather's great today. Want to go shopping?\"", "Lily: \"...Okay, I know you need to train your Persona.\""},
             {"Lily: \"Is Zhou's Yellow Braised Chicken really that good?\"", "Lily: \"Hmph, it's just chicken. What's so special about it?\"", "Lily: \"...Okay, fine, I secretly tried it too. It actually smells pretty good.\""},
             {"Lily: \"You've been spending so much time with other people lately...\"", "Lily: \"I was the one who met you first!\"", "Lily: \"...Just kidding! It's good that you have friends.\""},
             {"Lily: \"I found a secret garden behind the school!\"", "Lily: \"It's filled with glowing flowers, absolutely beautiful.\"", "Lily: \"But monsters come out at night. Will you go with me?\""},
             {"Lily: \"Actually, there are two sides to me.\"", "Lily: \"One is cheerful and lively during the day, the other... a bit dark at night.\"", "Lily: \"Which one do you like better?\""},
             {"Lily: \"I had that dream again last night.\"", "Lily: \"I dreamed you were wearing strange clothes, standing in line at a place called the 'Cafeteria'.\"", "Lily: \"...Is that thing called 'Yellow Braised Chicken' really that important?\""},
             {"Lily: \"Some strange people have been asking about you.\"", "Lily: \"They say they're your 'fellow townspeople', but something feels off.\"", "Lily: \"Be careful. I'll protect you!\""},
             {"Lily: \"If I learned how to make Yellow Braised Chicken, would you like me more?\"", "Lily: \"...Wait, why am I asking this!?\"", "Lily: \"Don't laugh! I... I was just asking!\""},
             {"Lily: \"You know, my Persona is 'The Lovers'.\"", "Lily: \"It represents bonds and choices.\"", "Lily: \"And my choice... has always been you.\""},
             {"Lily: \"Lately I've felt a strange energy within you.\"", "Lily: \"Like something is about to awaken.\"", "Lily: \"No matter what happens, I'll be by your side.\""},
             {"Lily: \"Finally... is it time to say goodbye?\"", "Lily: \"I know you have your own path to walk.\"", "Lily: \"But remember, no matter which world you're in, my heart goes with you. Safe travels, idiot.\""},
         }},
    };

    for (SocialLink *link : socialLinkManager_.allLinks())
    {
        if (!link)
            continue;
        auto it = kNpcDialogues.find(link->id());
        if (it == kNpcDialogues.end())
            continue;
        const auto &rankLines = it->second;
        for (int r = 0; r <= SocialLink::kMaxRank; ++r)
        {
            SocialLinkRankData data;
            if (r < static_cast<int>(rankLines.size()))
                data.dialogues = rankLines[r];
            else if (!rankLines.empty())
                data.dialogues = rankLines.back();
            else
                data.dialogues = {link->name() + ": \"...\""};
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

        std::vector<engine::Vec2> placedPositions;
        for (size_t i = 0; i < todayNpcIds_.size() && i < kTownNpcsPerDay; ++i)
        {
            engine::Vec2 pos = randomSpawnInZones(townNpcSpawnZones(), rng, placedPositions, 200.0f);
            const NpcDefinition *def = findNpc(todayNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i], spriteId));
            placedPositions.push_back(pos);
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

        std::vector<engine::Vec2> placedPositions;
        for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kSchoolNpcsPerDay; ++i)
        {
            engine::Vec2 pos = randomSpawnInZones(schoolNpcSpawnZones(), rng, placedPositions, 200.0f);
            const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i], spriteId));
            placedPositions.push_back(pos);
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
    std::vector<engine::Vec2> placedPositions;
    for (size_t i = 0; i < todayNpcIds_.size() && i < kTownNpcsPerDay; ++i)
    {
        engine::Vec2 pos = randomSpawnInZones(townNpcSpawnZones(), rng, placedPositions, 200.0f);
        const NpcDefinition *def = findNpc(todayNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        currentMap_->addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i], spriteId));
        placedPositions.push_back(pos);
    }
}

void GameManager::initSecondMap()
{
    secondMap_ = std::make_unique<TileMap>(25, 19);

    secondMap_->addEntity(std::make_unique<PlayerEntity>(schoolDefaultSpawn()));

    std::mt19937 rng(std::random_device{}());
    std::vector<engine::Vec2> placedPositions;
    for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kSchoolNpcsPerDay; ++i)
    {
        engine::Vec2 pos = randomSpawnInZones(schoolNpcSpawnZones(), rng, placedPositions, 200.0f);
        const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        secondMap_->addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i], spriteId));
        placedPositions.push_back(pos);
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
