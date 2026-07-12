#include "GameManager.h"
#include "Enemy.h"
#include "Item.h"
#include "MiscItem.h"
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
#include "QuestScene.h"

#include <algorithm>
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
    totalTalksToday_ = 0;
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

    // All default personas are owned by the player.
    character_.clearOwnedPersonas();
    for (const auto &p : personas_)
    {
        if (p)
            character_.addPersona(p);
    }

    // Pick a random balanced starter Persona.
    std::vector<std::string> starters = {
        "persona_izanagi", "persona_pixie", "persona_orpheus"};
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, starters.size() - 1);
    auto starter = findPersona(starters[dist(rng)]);
    if (starter)
        character_.setPersona(starter);

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
    bool ok = repo.saveAll(slotId, character_, inventory_, personas_,
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
    for (const auto &p : personas_)
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

    // Re-apply skills to the reconstructed Personas (loaded ones start empty).
    for (auto &p : personas_)
    {
        if (!p)
            continue;
        auto it = skillSnapshot.find(p->id());
        if (it != skillSnapshot.end())
        {
            for (const auto &s : it->second)
                p->learnSkill(s);
        }
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

    // Sync owned personas after load.
    character_.clearOwnedPersonas();
    for (const auto &p : personas_)
    {
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
    auto izanagi = std::make_shared<Persona>("persona_izanagi", "Izanagi", "Fool", 2,
                                             6, 5, 5);
    izanagi->setAffinity(Element::Electric, Affinity::Resist);
    izanagi->setAffinity(Element::Wind, Affinity::Weak);
    izanagi->learnSkill(std::make_shared<Skill>("skill_zio", "Zio", Element::Electric, 6, 4, SkillCostType::SP));
    izanagi->learnSkill(std::make_shared<Skill>("skill_cleave", "Cleave", Element::Physical, 8, 0, SkillCostType::HP));
    // Potential skills unlocked via Social Link rank-ups.
    izanagi->addPotentialSkill(3, std::make_shared<Skill>("skill_zionga", "Zionga", Element::Electric, 12, 8, SkillCostType::SP));
    izanagi->addPotentialSkill(6, std::make_shared<Skill>("skill_mazionga", "Mazionga", Element::Electric, 20, 14, SkillCostType::SP));
    personas_.push_back(izanagi);

    auto pixie = std::make_shared<Persona>("persona_pixie", "Pixie", "Magician", 2,
                                           4, 6, 5);
    pixie->setAffinity(Element::Wind, Affinity::Resist);
    pixie->setAffinity(Element::Electric, Affinity::Weak);
    pixie->learnSkill(std::make_shared<Skill>("skill_garu", "Garu", Element::Wind, 6, 4, SkillCostType::SP));
    pixie->learnSkill(std::make_shared<Skill>("skill_dia", "Dia", Element::Almighty, 15, 6, SkillCostType::SP, true));
    pixie->addPotentialSkill(3, std::make_shared<Skill>("skill_garula", "Garula", Element::Wind, 12, 8, SkillCostType::SP));
    pixie->addPotentialSkill(6, std::make_shared<Skill>("skill_diarama", "Diarama", Element::Almighty, 35, 12, SkillCostType::SP, true));
    personas_.push_back(pixie);

    auto orpheus = std::make_shared<Persona>("persona_orpheus", "Orpheus", "Fool", 2,
                                             5, 5, 5);
    orpheus->setAffinity(Element::Fire, Affinity::Resist);
    orpheus->setAffinity(Element::Ice, Affinity::Weak);
    orpheus->learnSkill(std::make_shared<Skill>("skill_agi", "Agi", Element::Fire, 6, 4, SkillCostType::SP));
    orpheus->learnSkill(std::make_shared<Skill>("skill_dia", "Dia", Element::Almighty, 15, 6, SkillCostType::SP, true));
    orpheus->addPotentialSkill(3, std::make_shared<Skill>("skill_agilao", "Agilao", Element::Fire, 12, 8, SkillCostType::SP));
    orpheus->addPotentialSkill(6, std::make_shared<Skill>("skill_diarama", "Diarama", Element::Almighty, 35, 12, SkillCostType::SP, true));
    personas_.push_back(orpheus);
}

void GameManager::initDefaultShop()
{
    shop_.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "A loaf of campus bread.", 10, 20, "items/bread"));
    shop_.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "Restores a lot of HP.", 30, 50, "items/potion_red"));
    shop_.addItem(std::make_unique<SpItem>("item_coffee", "Coffee", "Restores a little SP.", 20, 15, "items/coffee"));
    shop_.addItem(std::make_unique<PersonaItem>("item_pixie_contract", "Pixie Contract",
                                                "Summons Pixie.", 150, "persona_pixie"));
    // Quest collectibles
    shop_.addItem(std::make_unique<MiscItem>("item_star_shard", "Star Shard", "A crystallized fragment of memory dropped by monsters.", 80, "items/gem_blue"));
    shop_.addItem(std::make_unique<MiscItem>("item_moon_mushroom", "Moonlight Mushroom", "A glowing mushroom that only grows under the full moon.", 60, "items/mushroom"));
    shop_.addItem(std::make_unique<MiscItem>("item_monster_tooth", "Monster Tooth", "A sharp tooth taken from a defeated monster.", 40, "items/bone"));
    shop_.addItem(std::make_unique<MiscItem>("item_tarot_card", "Tarot Card", "A mysterious card imbued with arcane power. One of the 22 Major Arcana.", 100, "items/card"));
}

void GameManager::initDefaultQuests()
{
    // Fool - Yang Ming
    {
        Quest q1("quest_fool_1", "The Perfect Taste", "Bring 3 loaves of bread to Yang Ming.", "collect:bread:3", 30, 20);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_0");
        q1.setTargetItemId("food_bread");
        q1.setTargetCount(3);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_fool_2", "Mushroom Hunt", "Defeat 3 slimes for Yang Ming.", "kill:3", 50, 40);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_0");
        q2.setTargetCount(3);
        questManager_.addQuest(std::move(q2));
    }

    // Magician - Eric
    {
        Quest q1("quest_magician_1", "Arcane Study", "Defeat 2 goblins for Eric's research.", "kill:2", 40, 30);
        q1.setType(QuestType::Kill);
        q1.setNpcId("sl_npc_1");
        q1.setTargetCount(2);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_magician_2", "Star Shard Collection", "Bring Eric a Star Shard.", "collect:star_shard:1", 60, 50);
        q2.setType(QuestType::Collect);
        q2.setNpcId("sl_npc_1");
        q2.setTargetItemId("item_star_shard");
        q2.setTargetCount(1);
        questManager_.addQuest(std::move(q2));
    }

    // High Priestess - Selena
    {
        Quest q1("quest_priestess_1", "Moonlight Ritual", "Bring Selena 2 Moonlight Mushrooms.", "collect:moon_mushroom:2", 35, 25);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_2");
        q1.setTargetItemId("item_moon_mushroom");
        q1.setTargetCount(2);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_priestess_2", "Prophecy of Darkness", "Defeat a powerful monster for Selena's prophecy.", "kill:1", 100, 80);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_2");
        q2.setTargetCount(1);
        questManager_.addQuest(std::move(q2));
    }

    // Empress - Maria
    {
        Quest q1("quest_empress_1", "Warm Bread", "Bring Maria 3 loaves of bread.", "collect:bread:3", 25, 20);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_3");
        q1.setTargetItemId("food_bread");
        q1.setTargetCount(3);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_empress_2", "Exotic Flavors", "Defeat 4 goblins to collect rare ingredients.", "kill:4", 80, 70);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_3");
        q2.setTargetCount(4);
        questManager_.addQuest(std::move(q2));
    }

    // Emperor - Arthur
    {
        Quest q1("quest_emperor_1", "Clear the Slimes", "Defeat 5 slimes threatening the town.", "kill:5", 50, 40);
        q1.setType(QuestType::Kill);
        q1.setNpcId("sl_npc_4");
        q1.setTargetCount(5);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_emperor_2", "Arm the Guard", "Bring Arthur an iron sword.", "collect:iron_sword:1", 100, 80);
        q2.setType(QuestType::Collect);
        q2.setNpcId("sl_npc_4");
        q2.setTargetItemId("wp_iron");
        q2.setTargetCount(1);
        questManager_.addQuest(std::move(q2));
    }

    // Hierophant - Thomas
    {
        Quest q1("quest_hierophant_1", "Holy Offering", "Bring Thomas 3 HP potions.", "collect:potion:3", 45, 35);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_5");
        q1.setTargetItemId("potion_hp");
        q1.setTargetCount(3);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_hierophant_2", "Purify Evil", "Defeat 3 goblins for Thomas.", "kill:3", 90, 75);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_5");
        q2.setTargetCount(3);
        questManager_.addQuest(std::move(q2));
    }

    // Chariot - Maxim
    {
        Quest q1("quest_chariot_1", "Warrior's Proof", "Defeat 3 slimes for Maxim.", "kill:3", 60, 50);
        q1.setType(QuestType::Kill);
        q1.setNpcId("sl_npc_6");
        q1.setTargetCount(3);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_chariot_2", "Trophy Collection", "Bring Maxim 2 monster teeth.", "collect:monster_tooth:2", 55, 45);
        q2.setType(QuestType::Collect);
        q2.setNpcId("sl_npc_6");
        q2.setTargetItemId("item_monster_tooth");
        q2.setTargetCount(2);
        questManager_.addQuest(std::move(q2));
    }

    // Strength - Reina
    {
        Quest q1("quest_strength_1", "Feed the Beasts", "Bring Reina 4 loaves of bread.", "collect:bread:4", 30, 25);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_7");
        q1.setTargetItemId("food_bread");
        q1.setTargetCount(4);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_strength_2", "Challenge the Strong", "Defeat 2 powerful monsters for Reina.", "kill:2", 150, 120);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_7");
        q2.setTargetCount(2);
        questManager_.addQuest(std::move(q2));
    }

    // Hermit - Old Zhang
    {
        Quest q1("quest_hermit_1", "Bitter Memories", "Bring Old Zhang a coffee.", "collect:coffee:1", 20, 15);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_8");
        q1.setTargetItemId("item_coffee");
        q1.setTargetCount(1);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_hermit_2", "Cleansing the Past", "Defeat 5 goblins for Old Zhang.", "kill:5", 75, 65);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_8");
        q2.setTargetCount(5);
        questManager_.addQuest(std::move(q2));
    }

    // Wheel of Fortune - Fortuna
    {
        Quest q1("quest_fortune_1", "Gamble of Fate", "Bring Fortuna 2 Star Shards.", "collect:star_shard:2", 50, 40);
        q1.setType(QuestType::Collect);
        q1.setNpcId("sl_npc_9");
        q1.setTargetItemId("item_star_shard");
        q1.setTargetCount(2);
        questManager_.addQuest(std::move(q1));

        Quest q2("quest_fortune_2", "Guardian of Fate", "Defeat a powerful monster to protect the Wheel.", "kill:1", 120, 100);
        q2.setType(QuestType::Kill);
        q2.setNpcId("sl_npc_9");
        q2.setTargetCount(1);
        questManager_.addQuest(std::move(q2));
    }
}

void GameManager::initDefaultEnemies()
{
    enemyTemplates_.push_back(std::make_unique<Slime>());
    enemyTemplates_.push_back(std::make_unique<Goblin>());
    enemyTemplates_.push_back(std::make_unique<Boss>());
}

void GameManager::initDefaultSocialLinks()
{
    socialLinkManager_ = SocialLinkManager();
    generateNpcPool();
}

void GameManager::generateNpcPool()
{
    // Fixed pool of 10 tarot-themed NPCs. Each corresponds to a Major Arcana.
    // Names and arcana are fixed so story / quests remain stable across saves.
    static const std::vector<NpcDefinition> kFixedPool = {
        {"sl_npc_0", "Yang Ming", "npc_yosuke", "Fool"},
        {"sl_npc_1", "Eric", "npc_chie", "Magician"},
        {"sl_npc_2", "Selena", "npc_yukiko", "High Priestess"},
        {"sl_npc_3", "Maria", "npc_yosuke", "Empress"},
        {"sl_npc_4", "Arthur", "npc_chie", "Emperor"},
        {"sl_npc_5", "Thomas", "npc_yukiko", "Hierophant"},
        {"sl_npc_6", "Maxim", "npc_yosuke", "Chariot"},
        {"sl_npc_7", "Reina", "npc_chie", "Strength"},
        {"sl_npc_8", "Old Zhang", "npc_yukiko", "Hermit"},
        {"sl_npc_9", "Fortuna", "npc_yosuke", "Wheel of Fortune"}};

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
    // Each NPC has 11 entries (rank 0..10). Each entry is a vector of dialogue lines.
    static const std::map<std::string, std::vector<std::vector<std::string>>> kNpcDialogues = {
        // Fool - Yang Ming: a wanderer obsessed with the "perfect taste"
        {"sl_npc_0",
         {
             {"Yang Ming: \"Hey there! You look like someone who appreciates good food. Ever heard of... yellow-braised chicken?\"", "Yang Ming: \"I travel the world searching for flavors that remind me of home. That dish... it was perfection.\"", "Yang Ming: \"You also came from another world? I knew I wasn't the only one! The tarot cards... they hold the key.\""},
             {"Yang Ming: \"Hey, good to see you again! I've been thinking about that chicken you mentioned.\"", "Yang Ming: \"This world has strange ingredients, but nothing quite matches the golden sauce of home.\"", "Yang Ming: \"Every Persona carries a fragment of memory. Mine? It tastes like home.\""},
             {"Yang Ming: \"I met a chef once who could make miracles with mushrooms. Maybe we can find him?\"", "Yang Ming: \"The monsters here drop strange ingredients. I've been collecting them for a 'special recipe.'\"", "Yang Ming: \"You know, this world isn't so bad. At least the monsters don't complain about my cooking.\""},
             {"Yang Ming: \"I've been thinking... maybe the way back is through understanding all 22 Arcana.\"", "Yang Ming: \"Each Arcana represents a piece of the puzzle. Only together can they reveal the path home.\"", "Yang Ming: \"You're the only one who understands the longing for that flavor. Let's find it together.\""},
             {"Yang Ming: \"My Persona gets stronger every time I remember that taste. Memory is power here.\"", "Yang Ming: \"The Fool's journey is one of discovery. And today, I discovered a new recipe!\"", "Yang Ming: \"Whether we find that chicken again or not, I'm glad to have a friend who understands.\""},
             {"Yang Ming: \"I've been experimenting with monster ingredients. Want to try my latest creation?\"", "Yang Ming: \"It's not yellow-braised chicken, but it's got potential. Here, taste this!\"", "Yang Ming: \"Maybe the secret isn't finding the same dish, but creating something new that carries the same warmth.\""},
             {"Yang Ming: \"You know, I've been thinking about our journey. The 22 Arcana, the Personas, this whole world...\"", "Yang Ming: \"What if the reason we're here is because someone wanted us to find something?\"", "Yang Ming: \"Not just a way home, but a reason to stay. A purpose beyond the familiar.\""},
             {"Yang Ming: \"I found something interesting in the forest yesterday. A mushroom that glows golden.\"", "Yang Ming: \"It reminded me of the sauce from that chicken dish. Maybe... maybe it's connected.\"", "Yang Ming: \"The world works in mysterious ways. Even here, the memory of home finds a way to reach us.\""},
             {"Yang Ming: \"I've been talking to the other Arcana holders. They all have their own stories, their own reasons.\"", "Yang Ming: \"Some seek power, some seek knowledge. But you and I? We just want that taste of home.\"", "Yang Ming: \"And maybe that's exactly what makes our bonds the strongest. Shared longing creates the deepest ties.\""},
             {"Yang Ming: \"I've almost perfected the recipe. Just a few more ingredients, and I'll have something truly special.\"", "Yang Ming: \"It's not the same as the original, but it carries the same spirit. The same love.\"", "Yang Ming: \"When I share it with you, I want you to remember: home isn't a place, it's a feeling. And we can carry it anywhere.\""},
             {"Yang Ming: \"I've finally done it. I've recreated the yellow-braised chicken of my dreams!\"", "Yang Ming: \"Come, sit with me. Let's eat together, as friends who found each other across worlds.\"", "Yang Ming: \"No matter where our journey takes us next, we'll always have this moment. This taste. This bond.\""},
         }},
        // Magician - Eric: a scholar studying Personas
        {"sl_npc_1",
         {
             {"Eric: \"Ah, another awakened one. I can sense the resonance of your Persona from here.\"", "Eric: \"The Magician Arcana represents power and resourcefulness. It suits me, don't you think?\"", "Eric: \"I've been cataloguing the 22 Major Arcana Personas. Your collection is... impressive.\""},
             {"Eric: \"Each Persona corresponds to a skill. The Fool starts the journey; the World completes it.\"", "Eric: \"Did you know? The monsters at night carry crystallized memories. I call them 'Star Shards.'\"", "Eric: \"I need more data on combat applications. Would you help me study the monsters?\""},
             {"Eric: \"Your ability to switch Personas mid-battle is fascinating. Most people are bound to one.\"", "Eric: \"The tarot cards aren't just symbols. They're anchors for the soul in this world.\"", "Eric: \"I've made a breakthrough! The Star Shards can temporarily boost Persona abilities.\""},
             {"Eric: \"With your help, my research is almost complete. The truth about this world is within reach.\"", "Eric: \"But truth is like a Persona: the more you understand it, the more complex it becomes.\"", "Eric: \"Thank you, friend. You've helped me understand that knowledge means nothing without bonds.\""},
             {"Eric: \"I've been analyzing the patterns in Persona awakenings. There's a correlation between emotional state and power output.\"", "Eric: \"When you're thinking about that yellow-braised chicken, your Persona resonates at a unique frequency.\"", "Eric: \"Emotion is the key. The stronger the feeling, the stronger the Persona. Your love for that dish... it's your greatest power source.\""},
             {"Eric: \"The Arcana system is more complex than I initially thought. Each card doesn't just grant power—it shapes personality.\"", "Eric: \"The Fool isn't actually foolish. It represents the pure potential of a new beginning. Like you, arriving here with nothing but memories.\"", "Eric: \"And from that emptiness, infinite possibility. That's the true meaning of the Arcana.\""},
             {"Eric: \"I've discovered something troubling. The monsters aren't natural creatures.\"", "Eric: \"They're manifestations of... something. Fears, regrets, lost memories. Fragmented souls, given form.\"", "Eric: \"That explains why they drop crystallized memories. They're literally made of the stuff.\""},
             {"Eric: \"I've been collaborating with the Priestess. Her visions and my data are painting a clearer picture.\"", "Eric: \"This world... it's not just a world. It's a mirror. A reflection of collective human consciousness.\"", "Eric: \"The 22 Arcana represent 22 fundamental aspects of the human psyche. Understanding them means understanding ourselves.\""},
             {"Eric: \"My latest theory: the reason you can carry 10 Personas is because the human psyche has room for 10 archetypes.\"", "Eric: \"But you can only wield one at a time because the mind needs focus. Multiple active Personas would cause... instability.\"", "Eric: \"It's like having 10 recipes but only one kitchen. You can prepare them all, but cook only one at a time.\""},
             {"Eric: \"I've completed my research. The final piece of the puzzle... it's not what I expected.\"", "Eric: \"The 22 Arcana aren't separate entities. They're fragments of a single, greater whole.\"", "Eric: \"And that whole? It's not a Persona. It's a Person. Someone who embodies all 22 aspects simultaneously. Someone like... well, that's for you to discover.\""},
         }},
        // High Priestess - Selena: a mysterious fortune teller
        {"sl_npc_2",
         {
             {"Selena: \"The cards have foretold your arrival. The Fool who seeks the taste of home...\"", "Selena: \"I see visions in the moonlight. The Priestess gazes into the abyss and sees truth.\"", "Selena: \"Your destiny is intertwined with the 22 Arcana. Only by awakening them all will you find your way.\""},
             {"Selena: \"The night brings monsters, but also moonlight mushrooms. They glow with ancient memory.\"", "Selena: \"Beware the Shadow that walks between worlds. It hungers for what you cherish most.\"", "Selena: \"I have seen a vision: you and a plate of golden chicken, separated by a veil of stars.\""},
             {"Selena: \"The moon is full tonight. My visions are clearer than ever. Would you hear your fortune?\"", "Selena: \"Each NPC you meet is a fragment of the world's memory. We are all echoes of the tarot.\"", "Selena: \"Your bond with the Fool grows strong. Together, you may yet pierce the veil between worlds.\""},
             {"Selena: \"I see it now. The yellow-braised chicken is not just food. It is the anchor that calls you home.\"", "Selena: \"The moon reveals what the sun conceals. Tonight, the truth shines bright.\"", "Selena: \"You have grown much since we first met. The cards smile upon you, traveler.\""},
             {"Selena: \"I've been meditating on your future. The visions are... unusual.\"", "Selena: \"I see you standing at a crossroads. One path leads home, the other leads deeper into this world.\"", "Selena: \"But here's the secret: both paths are the same. Home is not a place you leave, but a place you carry with you.\""},
             {"Selena: \"The moon whispers secrets tonight. It speaks of a dish that transcends worlds.\"", "Selena: \"Your yellow-braised chicken... it's not just a meal. It's a memory encoded with love and longing.\"", "Selena: \"And in this world, where memories become magic, that love is a power beyond measure.\""},
             {"Selena: \"I've been reading the stars. They align in a pattern I've never seen before.\"", "Selena: \"The constellation of the Fool overlaps with the Chariot, the Hermit, and... the World.\"", "Selena: \"You're not just collecting Personas. You're assembling a complete soul. A unified self.\""},
             {"Selena: \"The other night, I had a vision of you at a table. Not in this world, but not in the old one either.\"", "Selena: \"A place between places. Where the 22 Arcana gather not as cards, but as friends.\"", "Selena: \"And at the center of that table? A steaming pot of yellow-braised chicken. The ultimate bond, shared among kindred spirits.\""},
             {"Selena: \"My visions are growing stronger. I can see the threads connecting all things now.\"", "Selena: \"The monsters, the Personas, the Arcana... they're all expressions of the same force. Consciousness, longing, the will to exist.\"", "Selena: \"And your longing for that simple dish? It's the purest expression of all. Love, made tangible.\""},
             {"Selena: \"I see a final vision. You, standing before a great door.\"", "Selena: \"Behind it lies your home. But the door doesn't open with a key. It opens with a memory.\"", "Selena: \"The memory of sitting with friends, sharing a warm meal, feeling completely at peace. That is the true key. And you've already forged it, in every bond you've made here.\""},
         }},
        // Empress - Maria: tavern owner whose cooking reminds the hero of home
        {"sl_npc_3",
         {
             {"Maria: \"Welcome to my tavern! I don't recognize your face, but you look like you could use a warm meal.\"", "Maria: \"I make the best bread in town. But sometimes... I feel like something is missing from my recipes.\"", "Maria: \"You say my bread reminds you of something called 'yellow-braised chicken'? How intriguing!\""},
             {"Maria: \"I dreamt of a dish with golden sauce and tender meat. Is that what you describe?\"", "Maria: \"Cooking is my Persona, in a way. It brings comfort and strength to those who partake.\"", "Maria: \"The monsters drop strange ingredients. I've been experimenting with them in my kitchen.\""},
             {"Maria: \"You know, the Emperor himself visits my tavern. Even he can't resist my cooking.\"", "Maria: \"I've created a new recipe! It doesn't taste like your chicken, but it has heart.\"", "Maria: \"The way your eyes light up when you talk about that dish... it must have been wonderful.\""},
             {"Maria: \"I'm going to keep trying until I recreate that 'yellow-braised chicken' for you.\"", "Maria: \"Friend, even if I never get it quite right, know that every meal here is made with care.\"", "Maria: \"Because in this world, food is more than sustenance. It's a way of showing love.\""},
             {"Maria: \"I've been thinking about your stories. About the world you came from.\"", "Maria: \"You say everyone there eats this 'yellow-braised chicken'? It must be a dish of great cultural significance.\"", "Maria: \"In this world, food that carries such meaning... it becomes magical. Literally. The love in the recipe becomes power in the eater.\""},
             {"Maria: \"I tried to recreate your dish last night. I used monster meat, golden mushrooms, and a sauce made from star nectar.\"", "Maria: \"It wasn't the same, of course. But when I tasted it... I felt something. A warmth. A connection across worlds.\"", "Maria: \"I think I understand now. The dish isn't important. The feeling it carries is what matters.\""},
             {"Maria: \"I've been talking to the other Arcana holders. They all have their own comfort foods, their own memories of home.\"", "Maria: \"The Fool misses his chicken. The Magician misses his library. The Hermit... well, he misses his solitude.\"", "Maria: \"But we all gather here, at my tavern, and share our memories through food. That's the true magic of this place.\""},
             {"Maria: \"I've created something special. A 'World's Welcome Stew.' It combines ingredients from every Arcana's homeland.\"", "Maria: \"When you taste it, you'll feel the love of the Fool, the wisdom of the Magician, the warmth of the Empress... all together.\"", "Maria: \"Because home isn't one place. It's the sum of all the places we've been loved.\""},
             {"Maria: \"I think I've finally cracked it. The secret to your yellow-braised chicken.\"", "Maria: \"It's not the ingredients. It's not the technique. It's the intention behind it.\"", "Maria: \"When someone cooks for you with love, the dish carries a piece of their soul. And that soul-food nourishes more than the body.\""},
             {"Maria: \"Tonight, I'm making a feast. For you, for all of us who found each other in this strange world.\"", "Maria: \"And at the center of the table, I'll place a dish. It may not be your yellow-braised chicken...\"", "Maria: \"But it's made with the same love. The same longing. The same hope. And I hope it tastes like home.\""},
         }},
        // Emperor - Arthur: town guard captain
        {"sl_npc_4",
         {
             {"Arthur: \"State your business. These streets aren't safe at night, what with the monsters about.\"", "Arthur: \"The Emperor stands for authority and protection. I keep order in this town.\"", "Arthur: \"I've heard rumors of a traveler from another world. You, perhaps? No matter. All are welcome if they follow the law.\""},
             {"Arthur: \"The night monsters grow bolder. I need capable fighters to cull their numbers.\"", "Arthur: \"My guards report strange crystals appearing near monster nests. Handle with care; they may be dangerous.\"", "Arthur: \"You've proven yourself in battle. Would you consider assisting the guard on a more permanent basis?\""},
             {"Arthur: \"An Emperor is only as strong as the foundation he builds. This town is my foundation.\"", "Arthur: \"I respect strength, traveler. But I respect compassion even more. Do not lose yours.\"", "Arthur: \"The monsters seem drawn to something in this town. I suspect there's a greater threat lurking.\""},
             {"Arthur: \"With your help, the town is safer than it's been in months. The people owe you a debt.\"", "Arthur: \"But safety is fragile. One weak link, and the whole chain breaks.\"", "Arthur: \"You have the heart of a true warrior. Should you ever need an ally, the guard stands with you.\""},
             {"Arthur: \"I've been reviewing the guard's reports. The monster attacks are becoming more coordinated.\"", "Arthur: \"They're not random anymore. Something is directing them. Something intelligent.\"", "Arthur: \"I need you to investigate the northern ruins. That's where the largest concentration of monsters has been spotted.\""},
             {"Arthur: \"The northern ruins... I should warn you. They're not just old buildings.\"", "Arthur: \"They're a gateway. A threshold between this world and... somewhere else.\"", "Arthur: \"The monsters are coming from there. And if we don't stop them, they'll overrun the entire region.\""},
             {"Arthur: \"My scouts returned from the ruins. They didn't make it far, but what they saw was troubling.\"", "Arthur: \"Giant crystals growing from the ground. Glowing with the same light as those Star Shards the monsters drop.\"", "Arthur: \"And in the center of it all, a figure. Not a monster. Something... else. Something waiting.\""},
             {"Arthur: \"I've been thinking about leadership. About what it means to be an Emperor.\"", "Arthur: \"It's not about giving orders. It's about taking responsibility. For every life under your protection.\"", "Arthur: \"You understand that. I can see it in the way you fight. Not for glory, but for others. That's true strength.\""},
             {"Arthur: \"We're preparing for a final push against the ruins. I want you there, at the front.\"", "Arthur: \"Not because you're the strongest. But because you fight for the right reasons. And that inspires others.\"", "Arthur: \"When this is over, I'll buy you that 'yellow-braised chicken' you keep talking about. Whatever it takes.\""},
             {"Arthur: \"The battle is won. The ruins are sealed. And you... you played the most crucial part.\"", "Arthur: \"I don't know if you realize this, but you're not just a traveler anymore. You're one of us now.\"", "Arthur: \"This town is your home. These people are your family. And as long as I stand, no one will take that from you.\""},
         }},
        // Hierophant - Thomas: old church priest who knows the lore
        {"sl_npc_5",
         {
             {"Thomas: \"Welcome, child. The church is open to all who seek wisdom, even those from distant... very distant lands.\"", "Thomas: \"The Hierophant holds the keys to tradition and hidden knowledge. I have studied the 22 Arcana for decades.\"", "Thomas: \"You carry the scent of another world. Do not be alarmed; the divine sees all paths.\""},
             {"Thomas: \"The tarot cards are more than images. They are living archetypes that shape reality itself.\"", "Thomas: \"I sense a great hunger in you, child. Not just for food, but for belonging.\"", "Thomas: \"The monsters are drawn to this town because of the Arcana's power. You must be cautious.\""},
             {"Thomas: \"I have prepared holy water for the faithful. Take some; it may protect you in the dark.\"", "Thomas: \"The Fool's journey is one of self-discovery. Every Arcana you meet is a mirror to your soul.\"", "Thomas: \"I have seen the signs. A great change is coming, and you are at the center of it.\""},
             {"Thomas: \"Your bonds with the townspeople grow stronger. That is the true magic of this world.\"", "Thomas: \"Not the spells or the Personas, but the connections between hearts. That is divine power.\"", "Thomas: \"May the light guide you, traveler. And may you find the home you seek.\""},
             {"Thomas: \"I've been studying the ancient texts. The 22 Arcana weren't always separate.\"", "Thomas: \"Long ago, they were one. A single, complete being. The World, in its truest sense.\"", "Thomas: \"But it shattered. Broke into 22 pieces. And those pieces became the Arcana we know today.\""},
             {"Thomas: \"The question is: why did it shatter? And more importantly, can it be reassembled?\"", "Thomas: \"Some texts suggest that the pieces can be reunited. But only by someone who embodies all 22 aspects.\"", "Thomas: \"Someone who has known joy and sorrow, strength and weakness, beginnings and endings. Someone like... well. You know who.\""},
             {"Thomas: \"I've been praying on your behalf. The divine has shown me visions of your journey.\"", "Thomas: \"I see you carrying 10 Personas, switching between them as needed. The Fool's versatility. The Magician's adaptability.\"", "Thomas: \"But I also see you choosing one to embody fully. Not just carrying it, but becoming it. That is the path to true mastery.\""},
             {"Thomas: \"The other Arcana holders speak of you with respect. Even the Emperor, who trusts few.\"", "Thomas: \"You have a way of connecting with people. Of seeing their true selves beneath the masks they wear.\"", "Thomas: \"That is the gift of the Fool. The ability to see truth in chaos. To find order in the random.\""},
             {"Thomas: \"I've had a troubling vision. The shadows are gathering. Something ancient stirs in the deep.\"", "Thomas: \"The 22 Arcana were created to seal it away. And if they're not strong enough... it will return.\"", "Thomas: \"But the texts also speak of a hero. A Fool from another world, who came not by choice but by destiny. Who carries the memory of home like a shield.\""},
             {"Thomas: \"You are that hero. I've known it since we first met.\"", "Thomas: \"Not because you're the strongest. But because you have the most to lose. And the most to protect.\"", "Thomas: \"The Arcana are ready. The town is ready. And I believe... you are ready too. Go forth, child. Your destiny awaits.\""},
         }},
        // Chariot - Maxim: hot-blooded adventurer seeking strong foes
        {"sl_npc_6",
         {
             {"Maxim: \"Hey! You look strong! Let's fight! ...No? Then at least tell me where the strong monsters are!\"", "Maxim: \"The Chariot charges forward without fear! That's my motto, and it should be yours too!\"", "Maxim: \"I've heard rumors of a traveler who fights with multiple Personas. That must be you!\""},
             {"Maxim: \"There's nothing better than a good fight to make you feel alive! Come on, let's spar!\"", "Maxim: \"The monsters at night are getting stronger. I couldn't be more thrilled!\"", "Maxim: \"I collect trophies from my victories. These monster teeth are proof of my strength!\""},
             {"Maxim: \"You ever notice how the monsters drop weird stuff? I saw one drop a glowing crystal once!\"", "Maxim: \"Strength isn't just about muscles. It's about the will to keep going no matter what!\"", "Maxim: \"I want to be the strongest in this world! And that means defeating the strongest monsters!\""},
             {"Maxim: \"You're a worthy rival, friend! One day we'll have an all-out battle, just you and me!\"", "Maxim: \"But not today. Today, we fight side by side. And that's even better!\"", "Maxim: \"No matter how strong I get, I'll never forget the friends who fought beside me. Thanks, partner!\""},
             {"Maxim: \"I've been training with the Emperor's guard. They taught me formations, strategy... boring stuff.\"", "Maxim: \"But they also taught me something useful: how to channel my Persona's power into my strikes.\"", "Maxim: \"When you equip a Persona, don't just use its power. Become its power. That's the Chariot's way!\""},
             {"Maxim: \"I fought a monster last night that could copy my moves. Every punch, every dodge.\"", "Maxim: \"At first, I was frustrated. But then I realized: if it can copy me, it means my moves are worth copying!\"", "Maxim: \"That's the secret to the Chariot. Don't fear being imitated. Fear being ignored.\""},
             {"Maxim: \"I've been collecting Star Shards. Not for selling, but for something better.\"", "Maxim: \"I grind them up and mix them into my armor. Makes it glow. Makes me feel... more.\"", "Maxim: \"The Magician says it's dangerous. I say it's awesome. And you know what? Being awesome is half the battle!\""},
             {"Maxim: \"The Emperor told me something interesting. About the ruins in the north.\"", "Maxim: \"He said there's a monster there that's stronger than anything we've faced. A real challenge!\"", "Maxim: \"I can't wait to fight it! But... I think I'll need your help. Not because I can't win, but because it'll be more fun together!\""},
             {"Maxim: \"I've been thinking about strength. Real strength.\"", "Maxim: \"It's not about being the strongest. It's about protecting what matters. And you've taught me that.\"", "Maxim: \"When I fight now, I don't just fight for myself. I fight for the town, for my friends, for that weird chicken dish you keep talking about.\""},
             {"Maxim: \"I've never said this before, but... I'm glad you're here.\"", "Maxim: \"This world was getting boring before you showed up. Same monsters, same battles. But you? You bring something new.\"", "Maxim: \"A reason to fight that's bigger than just winning. A reason to be strong. And I wouldn't have it any other way.\""},
         }},
        // Strength - Reina: beast tamer who lives in harmony with monsters
        {"sl_npc_7",
         {
             {"Reina: \"Shh, you'll scare the creatures. They're not as dangerous as they seem, once you understand them.\"", "Reina: \"The Strength Arcana isn't about brute force. It's about compassion and inner courage.\"", "Reina: \"I sense a gentle soul in you, despite all the fighting. That is true strength.\""},
             {"Reina: \"The monsters are just trying to survive, like us. But I understand the need to defend the town.\"", "Reina: \"I've been gathering food for the little ones. Could you spare some bread?\"", "Reina: \"My animal friends found something strange in the forest. Would you help me investigate?\""},
             {"Reina: \"True power comes from understanding, not domination. Remember that in your darkest moments.\"", "Reina: \"The monsters respect me because I respect them. It's a balance we must all find.\"", "Reina: \"You've shown both courage and kindness. That is the essence of the Strength Arcana.\""},
             {"Reina: \"My friends and I are safe because of you. Thank you for protecting the balance.\"", "Reina: \"But remember: every life is precious. Even the monsters have a place in this world.\"", "Reina: \"No matter how strong you become, never lose the compassion that makes you human.\""},
             {"Reina: \"I've been studying the monsters' behavior. They're not attacking randomly.\"", "Reina: \"They're fleeing something. Something in the north. Something that scares even them.\"", "Reina: \"If we can understand what they're running from, maybe we can help them. And help ourselves.\""},
             {"Reina: \"Last night, a monster cub wandered into town. It was lost, scared, alone.\"", "Reina: \"I fed it, comforted it, and guided it back to the forest. And you know what? It didn't attack anyone.\"", "Reina: \"Violence begets violence. But kindness... kindness can break the cycle.\""},
             {"Reina: \"The other Arcana holders don't understand me. They think I'm weak for not wanting to fight.\"", "Reina: \"But I don't avoid conflict out of fear. I avoid it out of respect. For life, for balance, for the sanctity of existence.\"", "Reina: \"True Strength isn't about defeating enemies. It's about having the power to destroy, and choosing not to.\""},
             {"Reina: \"I've been working on something. A way to communicate with the monsters.\"", "Reina: \"Not words, exactly. More like... emotions. Intentions. The universal language of the heart.\"", "Reina: \"And I think I've figured out how to teach it to you. Interested?\""},
             {"Reina: \"The Emperor asked me to help with the northern campaign. I agreed, but on one condition.\"", "Reina: \"That we try to spare the monsters who surrender. That we don't kill indiscriminately.\"", "Reina: \"He agreed. Grudgingly, but he agreed. That's progress. That's the Strength Arcana at work.\""},
             {"Reina: \"You've shown me that the world can be a better place. Not through power, but through understanding.\"", "Reina: \"The monsters and humans don't have to be enemies. We can coexist. We can thrive together.\"", "Reina: \"And when the dust settles, when peace finally comes... I hope you'll still visit. The creatures miss you already.\""},
         }},
        // Hermit - Old Zhang: elderly hermit who knows secrets
        {"sl_npc_8",
         {
             {"Old Zhang: \"Heh, another young one lost in this world. Sit a while. The road is long.\"", "Old Zhang: \"The Hermit seeks truth in solitude. I've lived alone so long, the stones speak to me.\"", "Old Zhang: \"You smell like... no, it can't be. Not here. Not yellow-braised chicken? Impossible!\""},
             {"Old Zhang: \"I knew someone like you, long ago. Came from nowhere, spoke of impossible things.\"", "Old Zhang: \"The answer you seek isn't in battle or magic. It's in the memories you hold dear.\"", "Old Zhang: \"Bring me something from the old world. Coffee, perhaps? It reminds me of better days.\""},
             {"Old Zhang: \"I've seen 22 Arcana come and go. Each one is a lesson. Each one is a blessing.\"", "Old Zhang: \"The night monsters fear the light of memory. Hold your truth close, and they cannot harm you.\"", "Old Zhang: \"You're searching for a way home, aren't you? I've searched for fifty years. Maybe together...\""},
             {"Old Zhang: \"You remind me of myself when I was young. Full of hope, full of fire. Don't lose that.\"", "Old Zhang: \"If you ever find that yellow-braised chicken, save a bite for an old man, will you?\"", "Old Zhang: \"The journey of a thousand miles begins with a single step. And yours... well, it's just getting interesting.\""},
             {"Old Zhang: \"I've been watching the stars. They tell a story, if you know how to read them.\"", "Old Zhang: \"Your star... it's not from this sky. It fell here, just like you. Drawn by something. Or someone.\"", "Old Zhang: \"I think you were brought here for a reason. Not by accident. By design. By the very fabric of this world.\""},
             {"Old Zhang: \"I've studied the 22 Arcana my whole life. And I know their secret now.\"", "Old Zhang: \"They're not just power sources. They're keys. Keys to a door that was sealed long ago.\"", "Old Zhang: \"And the keyhole? It's shaped like a heart. A heart full of longing for something lost. Like your chicken.\""},
             {"Old Zhang: \"The ruins in the north... I've been there. Long ago, before the monsters came.\"", "Old Zhang: \"There was a temple. And in that temple, a door. And behind that door... well. I never found out.\"", "Old Zhang: \"But I felt something. A presence. Ancient, powerful, and somehow... familiar. Like a memory I couldn't place.\""},
             {"Old Zhang: \"I've been teaching myself to cook, you know. Tried to recreate that chicken of yours.\"", "Old Zhang: \"Failed miserably, of course. But the trying... the trying was fun. It reminded me that it's never too late to learn.\"", "Old Zhang: \"And learning is the only thing that never gets old. Not even when you're as old as me.\""},
             {"Old Zhang: \"I've had a vision. You, standing before that door in the northern temple.\"", "Old Zhang: \"And it's opening. Not because you found the key, but because you finally understood what the key was.\"", "Old Zhang: \"Your love for that dish. Your longing for home. That's the key. It always was.\""},
             {"Old Zhang: \"I won't live forever. But I've lived long enough to see what matters.\"", "Old Zhang: \"It's not power. It's not knowledge. It's the bonds we forge. The memories we share. The love we give.\"", "Old Zhang: \"And when you finally return home, tell them about me. Tell them that somewhere, in a strange world, an old man remembers you. And wishes you well.\""},
         }},
        // Wheel of Fortune - Fortuna: mysterious merchant
        {"sl_npc_9",
         {
             {"Fortuna: \"Welcome, welcome! Fortune favors the bold... and those with coin to spend!\"", "Fortuna: \"The Wheel turns for everyone, dearie. Today you may be up, tomorrow down. That's life!\"", "Fortuna: \"I trade in fate and fancy. Got any Star Shards? They're quite valuable, you know.\""},
             {"Fortuna: \"I saw your arrival in my crystal ball. The Fool, wandering between worlds. How exciting!\"", "Fortuna: \"Everything has a price. Information, power, even memories... especially memories of home.\"", "Fortuna: \"The monsters hoard shiny things. I've made a fortune buying their trinkets from adventurers.\""},
             {"Fortuna: \"Care to spin the wheel? No? Too bad. Your fortune would have been... interesting.\"", "Fortuna: \"I've met many travelers, but none who longed for chicken. You truly are unique.\"", "Fortuna: \"The Arcana are like cards in a game. Play them right, and you might just win your way home.\""},
             {"Fortuna: \"Your luck is changing, dearie. I can feel it in my bones. Great things await you!\"", "Fortuna: \"Remember: the wheel always turns. No matter how dark it gets, dawn will come.\"", "Fortuna: \"And when it does, I'll be here. Ready to trade. Ready to play. Ready to see what fortune brings.\""},
             {"Fortuna: \"I've been thinking about destiny. About whether anything is truly random.\"", "Fortuna: \"The Wheel of Fortune spins, yes. But what if it's not random? What if every spin is... guided?\"", "Fortuna: \"By memory. By longing. By the love of a simple dish that tastes like home. What if that's what spins the wheel?\""},
             {"Fortuna: \"I had a customer yesterday. Sold him a Star Shard. He used it to power his Persona.\"", "Fortuna: \"But before he left, he said something strange. He said the shard smelled like... chicken?\"", "Fortuna: \"I laughed. But now I'm wondering. What if the Star Shards are crystallized memories? And what if YOUR memories are the most powerful of all?\""},
             {"Fortuna: \"I've been tracking the market. Star Shard prices are skyrocketing. Everyone wants them.\"", "Fortuna: \"But you know what? I'm not selling mine. I'm saving them. For someone special.\"", "Fortuna: \"Someone who might need a little luck when they face the final challenge. Someone like... well. You know who.\""},
             {"Fortuna: \"The other Arcana holders come to me for advice. They want to know their fortune.\"", "Fortuna: \"I tell them what they want to hear. But with you? I tell you the truth. Because you can handle it.\"", "Fortuna: \"The truth is: the wheel is rigged. Always has been. But the house doesn't always win. Not when the player has a heart as pure as yours.\""},
             {"Fortuna: \"I've seen the final spin. The one that decides everything.\"", "Fortuna: \"It's not a game of chance. It's a game of choice. And the choice is yours.\"", "Fortuna: \"Stay and fight for this world, or return to your own. Both are valid. Both are brave. And neither is wrong.\""},
             {"Fortuna: \"Whatever you choose, dearie, know this: the wheel is grateful for your presence.\"", "Fortuna: \"You brought something to this world that it didn't have before. A reason to spin. A reason to hope.\"", "Fortuna: \"And if you ever need a friend, a trader, or just someone to watch the wheel with... you know where to find me.\""},
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

        for (size_t i = 0; i < todayNpcIds_.size() && i < kTownNpcsPerDay; ++i)
        {
            engine::Vec2 pos = randomSpawnInZones(townNpcSpawnZones(), rng);
            map.addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i]));
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
            map.addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i]));
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

    // Check daily total unique-NPC cap first.
    if (totalTalksToday_ >= kMaxTalksPerDay)
    {
        std::string name = link ? link->name() : "...";
        return name + " nods politely, but you've already talked to two people today. Rest and try again tomorrow.";
    }

    // Hard cap: each NPC can be talked to kMaxTalksPerNpc times per day.
    if (count >= kMaxTalksPerNpc)
    {
        std::string name = link ? link->name() : "...";
        return name + " has nothing more to say today. Come back tomorrow.";
    }

    int beforeRank = link ? link->rank() : 0;
    socialLinkManager_.addPoints(socialLinkId, kDailyPoints);
    ++count;
    ++totalTalksToday_;

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
        currentMap_->addEntity(std::make_unique<NpcEntity>(pos, todayNpcIds_[i]));
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
        secondMap_->addEntity(std::make_unique<NpcEntity>(pos, todaySchoolNpcIds_[i]));
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
