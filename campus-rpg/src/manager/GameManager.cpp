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

#include <map>

GameManager &GameManager::instance()
{
    static GameManager manager;
    return manager;
}

void GameManager::seedDefaultState(const std::string &playerName)
{
    character_ = Character(playerName, 100, 50, 10, 10, 10, 10, 10);
    inventory_ = Inventory(20);
    shop_ = Shop();
    questManager_ = QuestManager();
    socialLinkManager_ = SocialLinkManager();
    enemyTemplates_.clear();
    personas_.clear();
    isNight_ = false;

    initDefaultPersonas();
    initDefaultShop();
    initDefaultQuests();
    initDefaultEnemies();
    initDefaultSocialLinks();
    initDefaultMap();

    // Equip starter Persona.
    auto starter = findPersona("persona_izanagi");
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
            return enemy->clone();
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

void GameManager::initDefaultPersonas()
{
    auto izanagi = std::make_shared<Persona>("persona_izanagi", "Izanagi", "Fool", 2,
                                             6, 5, 4, 5, 4);
    izanagi->setAffinity(Element::Electric, Affinity::Resist);
    izanagi->setAffinity(Element::Wind, Affinity::Weak);
    izanagi->learnSkill(std::make_shared<Skill>("skill_zio", "Zio", Element::Electric, 6, 4, SkillCostType::SP));
    izanagi->learnSkill(std::make_shared<Skill>("skill_cleave", "Cleave", Element::Physical, 8, 0, SkillCostType::HP));
    personas_.push_back(izanagi);

    auto pixie = std::make_shared<Persona>("persona_pixie", "Pixie", "Magician", 2,
                                           3, 6, 3, 6, 5);
    pixie->setAffinity(Element::Wind, Affinity::Resist);
    pixie->setAffinity(Element::Electric, Affinity::Weak);
    pixie->learnSkill(std::make_shared<Skill>("skill_garu", "Garu", Element::Wind, 6, 4, SkillCostType::SP));
    pixie->learnSkill(std::make_shared<Skill>("skill_dia", "Dia", Element::Almighty, 15, 6, SkillCostType::SP, true));
    personas_.push_back(pixie);
}

void GameManager::initDefaultShop()
{
    shop_.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "A loaf of campus bread.", 10, 20));
    shop_.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "Restores a lot of HP.", 30, 50));
    shop_.addItem(std::make_unique<SpItem>("item_coffee", "Coffee", "Restores a little SP.", 20, 15));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_wooden_sword", "Wooden Sword",
                                                  "A training sword.", 80, 5, 0, 0));
    shop_.addItem(std::make_unique<EquipmentItem>("eq_leather_armor", "Leather Armor",
                                                  "Basic protection.", 60, 0, 4, 0));
    shop_.addItem(std::make_unique<PersonaItem>("item_pixie_contract", "Pixie Contract",
                                                "Summons Pixie.", 150, "persona_pixie"));
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
    socialLinkManager_.addLink(SocialLink("sl_yosuke", "Yosuke", "Magician"));
    socialLinkManager_.addLink(SocialLink("sl_chie", "Chie", "Chariot"));
    socialLinkManager_.addLink(SocialLink("sl_yukiko", "Yukiko", "Priestess"));
    initSocialLinkRankData();
}

void GameManager::initSocialLinkRankData()
{
    auto fill = [](SocialLink &link, std::vector<std::string> dialogues)
    {
        for (int r = 0; r <= SocialLink::kMaxRank && r < static_cast<int>(dialogues.size()); ++r)
        {
            SocialLinkRankData data;
            data.dialogue = dialogues[r];
            link.setRankData(r, std::move(data));
        }
    };

    // Yosuke — Magician. Reward at rank 3: +1 Strength. Rank 6: +2 Strength. Rank 9: +3 Strength.
    if (SocialLink *y = socialLinkManager_.getLink("sl_yosuke"))
    {
        fill(*y, {
                     "Yosuke: \"哼，终于来了吗？本大爷可是等了很久——算了，勉强承认你是特别的存在吧。\"",
                     "Yosuke: \"哦？居然真的赴约了？有趣……你这家伙，和普通人不太一样。\"",
                     "Yosuke: \"听好了！在本大爷面前不用藏着掖着，你的黑暗面——我全部接受了！\"",
                     "Yosuke: \"并肩作战到这种地步，就算是神也拆不散我们的羁绊了！\"",
                     "Yosuke: \"喂喂，别小看我啊！我可是要成为最强的男人，才不是什么配角！\"",
                     "Yosuke: \"前方就算是地狱，有你在的话——不，是有我在，就一定能冲破！\"",
                     "Yosuke: \"哈……真是败给你了。这辈子，就认定你这个挚友了。\"",
                     "Yosuke: \"我从未想过，会有人让我想要变得更强……为了守护你。\"",
                     "Yosuke: \"哪怕世界毁灭，只要我们还在，希望就绝不会熄灭！\"",
                     "Yosuke: \"真正的兄弟，是连灵魂都能互相吞噬的存在——来吧，一起！\"",
                     "Yosuke: \"这就是……真正的『羁绊』吗。好热……胸口像被火焰燃烧一样！\"",
                 });
        if (SocialLinkRankData *r3 = y->rankData(3))
        {
            r3->reward.hasStatBonus = true;
            r3->reward.stat = PersonaStat::Strength;
            r3->reward.statBonus = 1;
        }
        if (SocialLinkRankData *r6 = y->rankData(6))
        {
            r6->reward.hasStatBonus = true;
            r6->reward.stat = PersonaStat::Strength;
            r6->reward.statBonus = 2;
        }
        if (SocialLinkRankData *r9 = y->rankData(9))
        {
            r9->reward.hasStatBonus = true;
            r9->reward.stat = PersonaStat::Strength;
            r9->reward.statBonus = 3;
        }
    }

    // Chie — Chariot. Reward at rank 3: +1 Agility. Rank 6: +2 Agility. Rank 9: +3 Agility.
    if (SocialLink *c = socialLinkManager_.getLink("sl_chie"))
    {
        fill(*c, {
                     "Chie: \"哼，来得正好！本小姐正想活动筋骨——可别拖后腿啊！\"",
                     "Chie: \"不错嘛，居然能跟上我的节奏。勉强承认你有两下子好了。\"",
                     "Chie: \"听着！和我组队的话，就必须有超越极限的觉悟！准备好了吗？\"",
                     "Chie: \"我会把想要伤害你的家伙全部粉碎——这就是我的『守护』之道！\"",
                     "Chie: \"哈！看到没有？这就是我和你的组合技，天下无敌！\"",
                     "Chie: \"挡在我们面前的东西，不管是谁——统统踢飞就好了吧！\"",
                     "Chie: \"力量不是用来欺凌弱小的……我要用这双手，保护所有重要的人！\"",
                     "Chie: \"谢谢……谢谢你一直相信我。作为回报，我会变得更强——强到让你依赖！\"",
                     "Chie: \"走过的路绝不会白费。每一步，都是我们『羁绊』的证明！\"",
                     "Chie: \"既是宿敌也是挚友吗……哈，真不赖。这辈子就让你当我一辈子的对手吧！\"",
                     "Chie: \"燃烧吧！直到最后一刻都不许倒下——因为我会把你拉起来！\"",
                 });
        if (SocialLinkRankData *r3 = c->rankData(3))
        {
            r3->reward.hasStatBonus = true;
            r3->reward.stat = PersonaStat::Agility;
            r3->reward.statBonus = 1;
        }
        if (SocialLinkRankData *r6 = c->rankData(6))
        {
            r6->reward.hasStatBonus = true;
            r6->reward.stat = PersonaStat::Agility;
            r6->reward.statBonus = 2;
        }
        if (SocialLinkRankData *r9 = c->rankData(9))
        {
            r9->reward.hasStatBonus = true;
            r9->reward.stat = PersonaStat::Agility;
            r9->reward.statBonus = 3;
        }
    }

    // Yukiko — Priestess. Reward at rank 3: +1 Magic. Rank 6: +2 Magic. Rank 9: +3 Magic.
    if (SocialLink *y = socialLinkManager_.getLink("sl_yukiko"))
    {
        fill(*y, {
                     "Yukiko: \"啊……欢迎。茶已经准备好了。请、请坐吧……\"",
                     "Yukiko: \"和你在一起的时候，心里就会变得很平静……这是为什么呢？\"",
                     "Yukiko: \"最近总是在想……如果我能再勇敢一点，是不是就能更接近你了？\"",
                     "Yukiko: \"你……你愿意告诉我，真正的『自由』是什么吗？我想知道。\"",
                     "Yukiko: \"家族、传统、束缚……那些东西，我开始想用自己的方式去理解了。\"",
                     "Yukiko: \"谢谢你……一直陪在我身边。这份温暖，我会永远珍藏的。\"",
                     "Yukiko: \"我也想……成为能支撑你的人。不是一直被保护，而是可以并肩前行。\"",
                     "Yukiko: \"已经不怕了。因为只要你在，就算黑暗也会变得温柔……对吧？\"",
                     "Yukiko: \"无论未来怎样，我都会微笑着面对。因为……你教给了我勇气。\"",
                     "Yukiko: \"你是……我最重要的人。比任何事物都要珍贵。\"",
                     "Yukiko: \"让我们走吧……一起，走向那道光芒的尽头。我会一直、一直在你身边。\"",
                 });
        if (SocialLinkRankData *r3 = y->rankData(3))
        {
            r3->reward.hasStatBonus = true;
            r3->reward.stat = PersonaStat::Magic;
            r3->reward.statBonus = 1;
        }
        if (SocialLinkRankData *r6 = y->rankData(6))
        {
            r6->reward.hasStatBonus = true;
            r6->reward.stat = PersonaStat::Magic;
            r6->reward.statBonus = 2;
        }
        if (SocialLinkRankData *r9 = y->rankData(9))
        {
            r9->reward.hasStatBonus = true;
            r9->reward.stat = PersonaStat::Magic;
            r9->reward.statBonus = 3;
        }
    }
}

std::string GameManager::talkToNpc(const std::string &socialLinkId)
{
    const int kDailyPoints = 10;
    int beforeRank = 0;
    if (const SocialLink *before = socialLinkManager_.getLink(socialLinkId))
        beforeRank = before->rank();

    socialLinkManager_.addPoints(socialLinkId, kDailyPoints);

    // Fire the rank-up callback for every rank gained during this talk so
    // the UI can play the 奶龙 laugh / fire-dance sound and show a banner.
    if (rankUpCallback_)
    {
        const SocialLink *after = socialLinkManager_.getLink(socialLinkId);
        if (after && after->rank() > beforeRank)
        {
            for (int r = beforeRank + 1; r <= after->rank(); ++r)
                rankUpCallback_(socialLinkId, r);
        }
    }

    recomputeSocialLinkBonuses();
    return socialLinkManager_.dialogueFor(socialLinkId);
}

void GameManager::recomputeSocialLinkBonuses()
{
    character_.clearSocialLinkBonuses();

    static const PersonaStat stats[] = {
        PersonaStat::Strength, PersonaStat::Magic, PersonaStat::Endurance,
        PersonaStat::Agility, PersonaStat::Luck};
    std::vector<std::string> arcanas;
    for (const auto *link : socialLinkManager_.allLinks())
    {
        if (link && std::find(arcanas.begin(), arcanas.end(), link->arcana()) == arcanas.end())
            arcanas.push_back(link->arcana());
    }
    for (const std::string &arcana : arcanas)
    {
        for (PersonaStat s : stats)
        {
            int bonus = socialLinkManager_.arcanaStatBonus(arcana, personaStatName(s));
            if (bonus != 0)
                character_.applySocialLinkBonus(s, bonus);
        }
    }
}

void GameManager::initDefaultMap()
{
    currentMap_ = std::make_unique<TileMap>(20, 15);

    // Simple border walls.
    for (int x = 0; x < 20; ++x)
    {
        currentMap_->tileAt(x, 0) = Tile(TileType::Wall);
        currentMap_->tileAt(x, 14) = Tile(TileType::Wall);
    }
    for (int y = 0; y < 15; ++y)
    {
        currentMap_->tileAt(0, y) = Tile(TileType::Wall);
        currentMap_->tileAt(19, y) = Tile(TileType::Wall);
    }

    // Some interior walls.
    currentMap_->tileAt(5, 5) = Tile(TileType::Wall);
    currentMap_->tileAt(5, 6) = Tile(TileType::Wall);
    currentMap_->tileAt(14, 8) = Tile(TileType::Wall);
    currentMap_->tileAt(14, 9) = Tile(TileType::Wall);

    // Player start.
    currentMap_->addEntity(std::make_unique<PlayerEntity>(engine::Vec2{100.0f, 100.0f}));

    // NPCs for Social Link.
    currentMap_->addEntity(std::make_unique<NpcEntity>(engine::Vec2{200.0f, 150.0f}, "sl_yosuke"));
    currentMap_->addEntity(std::make_unique<NpcEntity>(engine::Vec2{300.0f, 200.0f}, "sl_chie"));
    currentMap_->addEntity(std::make_unique<NpcEntity>(engine::Vec2{150.0f, 250.0f}, "sl_yukiko"));
}
