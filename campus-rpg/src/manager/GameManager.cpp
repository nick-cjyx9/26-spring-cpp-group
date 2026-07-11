#include "GameManager.h"
#include "Enemy.h"
#include "Item.h"
#include "Persona.h"
#include "SaveRepository.h"
#include "DatabaseManager.h"

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

#include <algorithm>
#include <map>
#include <random>

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
    SaveRepository repo;
    bool ok = repo.saveAll(slotId, character_, inventory_, personas_,
                           socialLinkManager_, questManager_);
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

    if (!repo.loadAll(slotId, character_, inventory_, personas_,
                      socialLinkManager_, questManager_))
    {
        // Load failed: leave the seeded default state intact so the game is
        // still in a runnable state.
        return false;
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
    }
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

void GameManager::addPersonaToPlayer(std::shared_ptr<Persona> persona)
{
    if (!persona)
        return;
    if (!findPersona(persona->id()))
        personas_.push_back(persona);
    character_.addPersona(persona);
}

void GameManager::setPlayerPersona(std::shared_ptr<Persona> persona)
{
    if (!persona)
        return;
    addPersonaToPlayer(persona);
    character_.setPersona(persona);
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
    shop_.addItem(std::make_unique<EquipmentItem>("eq_wooden_sword", "Wooden Sword",
                                                  "A training sword.", 80, 5, 0, 0,
                                                  EquipmentSlot::Weapon, "tile_00_00"));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_leather_armor", "Leather Armor",
                                                  "Basic protection.", 60, 0, 4, 0,
                                                  EquipmentSlot::Armor, "tile_15_24"));
    shop_.addItem(std::make_unique<PersonaItem>("item_pixie_contract", "Pixie Contract",
                                                "Summons Pixie.", 150, "persona_pixie", "items/persona_pixie"));
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
    // Pick kNpcsPerDay for town and kNpcsPerDay different ones for school,
    // so within a day both maps show distinct NPCs.
    int totalNeeded = kNpcsPerDay * 2;
    for (int i = 0; i < totalNeeded && i < static_cast<int>(ids.size()); ++i)
    {
        if (i < kNpcsPerDay)
            todayNpcIds_.push_back(ids[i]);
        else
            todaySchoolNpcIds_.push_back(ids[i]);
    }
}

void GameManager::rebuildMapNpcs()
{
    // Town map
    if (currentMap_)
    {
        TileMap &map = *currentMap_;
        engine::Vec2 playerPos{100.0f, 100.0f};
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                playerPos = e->position();
        }
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));

        static const engine::Vec2 kSpots[kNpcsPerDay] = {
            engine::Vec2{200.0f, 150.0f}, engine::Vec2{300.0f, 200.0f}};
        for (size_t i = 0; i < todayNpcIds_.size() && i < kNpcsPerDay; ++i)
        {
            const NpcDefinition *def = findNpc(todayNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(kSpots[i], todayNpcIds_[i], spriteId));
        }
    }

    // School map (second map)
    if (secondMap_)
    {
        TileMap &map = *secondMap_;
        engine::Vec2 playerPos{100.0f, 100.0f};
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
                playerPos = e->position();
        }
        map.clearEntities();
        map.addEntity(std::make_unique<PlayerEntity>(playerPos));

        static const engine::Vec2 kSpots[kNpcsPerDay] = {
            engine::Vec2{250.0f, 180.0f}, engine::Vec2{400.0f, 230.0f}};
        for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kNpcsPerDay; ++i)
        {
            const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
            std::string spriteId = def ? def->spriteId : "";
            map.addEntity(std::make_unique<NpcEntity>(kSpots[i], todaySchoolNpcIds_[i], spriteId));
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
    // 25x19 tiles * 32px = 800x600, matching the full window. No walls: the
    // player can walk anywhere on the background image.
    currentMap_ = std::make_unique<TileMap>(25, 19);

    // Player start.
    currentMap_->addEntity(std::make_unique<PlayerEntity>(engine::Vec2{100.0f, 100.0f}));

    // Today's NPCs (kNpcsPerDay of them, picked by refreshDailyNpcs()).
    static const engine::Vec2 kNpcSpots[kNpcsPerDay] = {
        engine::Vec2{200.0f, 150.0f}, engine::Vec2{300.0f, 200.0f}};
    for (size_t i = 0; i < todayNpcIds_.size() && i < kNpcsPerDay; ++i)
    {
        const NpcDefinition *def = findNpc(todayNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        currentMap_->addEntity(std::make_unique<NpcEntity>(kNpcSpots[i], todayNpcIds_[i], spriteId));
    }
}

void GameManager::initSecondMap()
{
    // Full 800x600 walkable area, no walls.
    secondMap_ = std::make_unique<TileMap>(25, 19);

    // Player start.
    secondMap_->addEntity(std::make_unique<PlayerEntity>(engine::Vec2{100.0f, 100.0f}));

    // Today's school NPCs.
    static const engine::Vec2 kNpcSpots[kNpcsPerDay] = {
        engine::Vec2{250.0f, 180.0f}, engine::Vec2{400.0f, 230.0f}};
    for (size_t i = 0; i < todaySchoolNpcIds_.size() && i < kNpcsPerDay; ++i)
    {
        const NpcDefinition *def = findNpc(todaySchoolNpcIds_[i]);
        std::string spriteId = def ? def->spriteId : "";
        secondMap_->addEntity(std::make_unique<NpcEntity>(kNpcSpots[i], todaySchoolNpcIds_[i], spriteId));
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
