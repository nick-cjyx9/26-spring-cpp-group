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

GameManager &GameManager::instance()
{
    static GameManager manager;
    return manager;
}

void GameManager::newGame(const std::string &playerName)
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

    enterScene(SceneType::Title);
}

void GameManager::save()
{
    SaveRepository repo;
    repo.saveAll(character_, inventory_, personas_, socialLinkManager_, questManager_);
}

void GameManager::load()
{
    SaveRepository repo;
    repo.loadAll(character_, inventory_, personas_, socialLinkManager_, questManager_);

    std::string currentId = character_.currentPersona() ? character_.currentPersona()->id() : "";
    auto p = findPersona(currentId);
    if (p)
        character_.setPersona(p);
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
    }
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
    socialLinkManager_.addLink(SocialLink("sl_yosuke", "Yosuke", "Magician"));
    socialLinkManager_.addLink(SocialLink("sl_chie", "Chie", "Chariot"));
    socialLinkManager_.addLink(SocialLink("sl_yukiko", "Yukiko", "Priestess"));
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
