// CampusRPG core unit tests — pure C++ lightweight runner (no SFML/SQLite dependency).

#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "MiscItem.h"
#include "Enemy.h"
#include "Shop.h"
#include "BattleSystem.h"
#include "Persona.h"
#include "Skill.h"
#include "Element.h"
#include "Affinity.h"
#include "SocialLink.h"
#include "SocialLinkManager.h"
#include "Entity.h"
#include "TileMap.h"
#include "Quest.h"
#include "QuestManager.h"

#include <iostream>
#include <string>

namespace
{
    int g_run = 0;
    int g_failed = 0;

    void record(bool ok, const char *expr, const char *file, int line)
    {
        ++g_run;
        if (!ok)
        {
            ++g_failed;
            std::cerr << "FAIL: " << expr << "  (" << file << ':' << line << ")\n";
        }
    }

#define CHECK(expr) record(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b) record((a) == (b), #a " == " #b, __FILE__, __LINE__)
} // namespace

std::shared_ptr<Persona> makeBasicPersona(const std::string &id, int str, int mag, int spd)
{
    return std::make_shared<Persona>(id, id, "Fool", 1, str, mag, spd);
}

void testCharacterGainExpTriggersLevelUp()
{
    Character hero("Hero", 100, 50);
    CHECK_EQ(hero.level(), 1);
    hero.gainExp(100);
    CHECK_EQ(hero.level(), 2);
}

void testCharacterLevelUpSnapshotPopulated()
{
    Character hero("Hero", 100, 50);
    CHECK(!hero.hasLevelUpSnapshot());
    hero.gainExp(100);
    CHECK(hero.hasLevelUpSnapshot());
    const auto &snap = hero.levelUpSnapshot();
    CHECK_EQ(snap.oldLevel, 1);
    CHECK_EQ(snap.newLevel, 2);
    CHECK(snap.newMaxHp > snap.oldMaxHp);
    CHECK(snap.newMaxSp > snap.oldMaxSp);
    hero.clearLevelUpSnapshot();
    CHECK(!hero.hasLevelUpSnapshot());
}

void testCharacterLevelUpIncreasesStats()
{
    Character hero("Hero", 100, 50);
    int initialMaxHp = hero.maxHp();
    hero.gainExp(100);
    CHECK(hero.maxHp() > initialMaxHp);
}

void testCharacterTakeDamageNoDefense()
{
    Character hero("Hero", 100, 50);
    hero.takeDamage(50);
    CHECK_EQ(hero.hp(), 50);
}

void testFoodItemHealsCharacter()
{
    Character hero("Hero", 100, 50);
    hero.takeDamage(50);
    FoodItem bread("food_bread", "Bread", "Tasty bread", 10, 30);
    bread.use(hero);
    CHECK(hero.hp() > 50);
}

void testEquipmentItemBoostsStrength()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 10, 10, 10));
    int baseAttack = hero.attack();
    EquipmentItem sword("eq_sword", "Sword", "Sharp sword", 50, 5, 0, 0, EquipmentSlot::Weapon);
    sword.use(hero);
    CHECK_EQ(hero.attack(), baseAttack + 5);
}

void testInventoryAddItemIncreasesSize()
{
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));
    CHECK_EQ(inv.size(), static_cast<std::size_t>(1));
}

void testInventoryUseItemAppliesEffectAndRemovesIt()
{
    Inventory inv;
    inv.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "Restores HP", 30, 50));
    Character hero("Hero", 100, 50);
    hero.takeDamage(60);
    int hpBefore = hero.hp();
    CHECK(inv.useItem(0, hero));
    CHECK(hero.hp() > hpBefore);
    CHECK(inv.empty());
}

void testShopBuyItemTransfersItemAndConsumesGold()
{
    Shop shop;
    shop.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));
    Character buyer("Buyer", 100, 50);
    buyer.addGold(100);
    Inventory inv;
    CHECK(shop.buy(0, buyer, inv));
    CHECK_EQ(inv.size(), static_cast<std::size_t>(1));
    CHECK_EQ(buyer.gold(), 90);
}

void testBattleStrongPlayerDefeatsSlime()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 20, 20, 20));
    Slime slime;
    BattleSystem battle;
    battle.startBattle(hero, slime);

    int safety = 0;
    while (!battle.isOver() && safety < 100)
    {
        if (battle.isPlayerTurn())
            battle.playerAttack();
        ++safety;
    }

    CHECK(battle.isOver());
    CHECK(battle.playerWon());
}

void testSocialLinkProgression()
{
    SocialLink link("sl_test", "Test", "Fool");
    CHECK_EQ(link.rank(), 0);
    link.addPoints(25);
    CHECK(link.rank() > 0);
}

void testSocialLinkAddPointsReturnsRanksGained()
{
    SocialLink link("sl_test", "Test", "Fool");
    // rank 0 -> needs (0+1)*20 = 20 points to rank 1.
    int gained = link.addPoints(20);
    CHECK_EQ(gained, 1);
    CHECK_EQ(link.rank(), 1);
    // rank 1 -> needs 40 more. Give 100 => 1->2 (40), 2->3 (60) = 2 ranks, 0 left.
    gained = link.addPoints(100);
    CHECK_EQ(gained, 2);
    CHECK_EQ(link.rank(), 3);
    CHECK_EQ(link.points(), 0);
}

void testSocialLinkMaxRankCapsAtTen()
{
    SocialLink link("sl_test", "Test", "Fool");
    // Flood with way more than enough to reach max rank.
    link.addPoints(99999);
    CHECK_EQ(link.rank(), SocialLink::kMaxRank);
    CHECK(link.isMaxRank());
    // Further points are absorbed but rank stays capped.
    link.addPoints(500);
    CHECK_EQ(link.rank(), SocialLink::kMaxRank);
}

void testSocialLinkRankDataDialogueAndReward()
{
    SocialLink link("sl_test", "Test", "Fool");
    link.setRankData(0, {"hello rank 0"});
    auto skill = std::make_shared<Skill>("skill_test", "Test Skill", Element::Fire, 10, 5, SkillCostType::SP);
    SocialLinkRankData rank1Data;
    rank1Data.dialogue = "hello rank 1";
    rank1Data.reward.personaLevels = 1;
    rank1Data.reward.newSkill = skill;
    link.setRankData(1, rank1Data);
    CHECK_EQ(link.currentDialogue(), "hello rank 0");
    link.addPoints(20); // -> rank 1
    CHECK_EQ(link.currentDialogue(), "hello rank 1");
    const SocialLinkReward *r = link.currentReward();
    CHECK(r != nullptr);
    CHECK_EQ(r->personaLevels, 1);
    CHECK(r->newSkill != nullptr);
}

void testSocialLinkManagerAddPointsNotifiesRankUp()
{
    SocialLinkManager mgr;
    mgr.addLink(SocialLink("sl_a", "A", "Magician"));
    // 0 -> 1 needs 20 points.
    CHECK_EQ(mgr.addPoints("sl_a", 20), 1);
    CHECK(mgr.hasPendingRankUps());
    auto pending = mgr.consumePendingRankUps();
    CHECK_EQ(pending.size(), static_cast<std::size_t>(1));
    CHECK_EQ(pending[0], "sl_a");
    CHECK(!mgr.hasPendingRankUps());
    // Non-existent link: no ranks, no pending.
    CHECK_EQ(mgr.addPoints("does_not_exist", 999), 0);
    CHECK(!mgr.hasPendingRankUps());
}

void testSocialLinkManagerDialogueForCurrentRank()
{
    SocialLinkManager mgr;
    SocialLink sl("sl_a", "A", "Fool");
    sl.setRankData(0, {"rank0 text"});
    sl.setRankData(1, {"rank1 text"});
    mgr.addLink(std::move(sl));
    CHECK_EQ(mgr.dialogueFor("sl_a"), "rank0 text");
    mgr.addPoints("sl_a", 20);
    CHECK_EQ(mgr.dialogueFor("sl_a"), "rank1 text");
    CHECK_EQ(mgr.dialogueFor("missing"), "");
}

void testPersonaLevelsFromSocialLink()
{
    auto persona = makeBasicPersona("p", 10, 10, 10);
    Character hero("Hero", 100, 50);
    hero.setPersona(persona);

    SocialLink link("sl_test", "Test", "Fool");
    link.setRankData(1, {.dialogue = "rank1", .reward = {.personaLevels = 1}});
    link.addPoints(20);

    int beforeLevel = persona->level();
    for (int i = 0; i < link.rank(); ++i)
    {
        const SocialLinkRankData *d = link.rankData(i + 1);
        if (d)
        {
            for (int j = 0; j < d->reward.personaLevels; ++j)
                persona->gainExp(persona->expToNextLevel());
        }
    }
    CHECK(persona->level() > beforeLevel);
}

void testEntityIntersection()
{
    PlayerEntity p({0, 0});
    NpcEntity n({10, 10}, "sl_test");
    CHECK(p.intersects(n));
}

void testTileMapWalkability()
{
    TileMap map(5, 5);
    map.tileAt(1, 1) = Tile(TileType::Wall);
    CHECK(!map.isWalkable(1, 1));
    CHECK(map.isWalkable(2, 2));
}

void testPersonaSkillUnlockOnLevelUp()
{
    Persona p("p_test", "Test", "Fool", 1, 5, 5, 5);
    auto skill = std::make_shared<Skill>("skill_test", "Test Skill", Element::Fire, 10, 5, SkillCostType::SP, false, 3);
    p.addPotentialSkill(3, skill);
    CHECK_EQ(p.skills().size(), static_cast<std::size_t>(0));
    // Level up from 1 to at least 3 (gainExp(999) will level up multiple times).
    p.gainExp(999);
    CHECK(p.level() >= 3);
    CHECK_EQ(p.skills().size(), static_cast<std::size_t>(1));
    CHECK(p.findSkill("skill_test") != nullptr);
}

// ---- Character / Persona ownership ----

void testCharacterPersonaOwnership()
{
    Character hero("Hero", 100, 50);
    auto p1 = makeBasicPersona("p1", 5, 5, 5);
    auto p2 = makeBasicPersona("p2", 6, 6, 6);

    CHECK(hero.addPersona(p1));
    CHECK(hero.addPersona(p2));
    CHECK_EQ(hero.ownedPersonas().size(), static_cast<std::size_t>(2));
    CHECK(hero.ownsPersona("p1"));
    CHECK(hero.ownsPersona("p2"));

    hero.setPersona(p1);
    CHECK_EQ(hero.currentPersona()->id(), std::string("p1"));

    CHECK(hero.removePersona("p1"));
    CHECK(!hero.ownsPersona("p1"));
    CHECK_EQ(hero.currentPersona()->id(), std::string("p2"));

    hero.clearOwnedPersonas();
    CHECK_EQ(hero.ownedPersonas().size(), static_cast<std::size_t>(0));
}

void testCharacterEquipmentBonuses()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 10, 10, 10));

    EquipmentItem armor("eq_armor", "Robe", "Magic robe", 50, 0, 7, 0, EquipmentSlot::Armor);
    EquipmentItem boots("eq_boots", "Boots", "Swift boots", 50, 0, 0, 6, EquipmentSlot::Accessory);

    armor.use(hero);
    boots.use(hero);

    CHECK_EQ(hero.equipmentMagicBonus(), 7);
    CHECK_EQ(hero.equipmentSpeedBonus(), 6);
    CHECK(hero.magic() > 10);
    CHECK(hero.speed() > 10);
}

void testCharacterSpAndGold()
{
    Character hero("Hero", 100, 50);
    hero.consumeSp(20);
    CHECK_EQ(hero.sp(), 30);
    hero.recoverSp(100);
    CHECK_EQ(hero.sp(), hero.maxSp());

    hero.addGold(100);
    CHECK(hero.spendGold(40));
    CHECK_EQ(hero.gold(), 60);
    CHECK(!hero.spendGold(100));
}

void testCharacterHpCannotGoNegative()
{
    Character hero("Hero", 100, 50);
    hero.takeDamage(999);
    CHECK_EQ(hero.hp(), 0);
}

// ---- Skill ----

void testSkillCanUseAndApplyCost()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 5, 5, 5));
    Skill fire("skill_agi", "Agi", Element::Fire, 10, 8, SkillCostType::SP);

    CHECK(fire.canUse(hero));
    fire.applyCost(hero);
    CHECK_EQ(hero.sp(), 42);

    Skill costly("skill_big", "Big", Element::Fire, 10, 200, SkillCostType::SP);
    CHECK(!costly.canUse(hero));
}

void testSkillClonePreservesFields()
{
    Skill original("skill_agi", "Agi", Element::Fire, 10, 8, SkillCostType::SP, false, 3);
    auto cloned = original.clone();
    CHECK_EQ(cloned->id(), original.id());
    CHECK_EQ(cloned->name(), original.name());
    CHECK_EQ(cloned->element(), original.element());
    CHECK_EQ(cloned->cost(), original.cost());
    CHECK_EQ(cloned->requiredLevel(), original.requiredLevel());
}

// ---- Item / Inventory ----

void testItemQuantityAndClone()
{
    FoodItem bread("food_bread", "Bread", "Tasty", 10, 20);
    bread.setQuantity(3);
    auto cloned = bread.clone();
    CHECK_EQ(cloned->quantity(), 3);
    cloned->addQuantity(2);
    CHECK_EQ(cloned->quantity(), 5);
}

void testMiscItemAndPersonaItemNoOp()
{
    Character hero("Hero", 100, 50);
    int hpBefore = hero.hp();
    MiscItem key("item_key", "Key", "A key", 0);
    key.use(hero);
    CHECK_EQ(hero.hp(), hpBefore);

    PersonaItem contract("item_contract", "Contract", "Summons", 100, "persona_test");
    contract.use(hero);
    CHECK_EQ(hero.hp(), hpBefore);
}

void testInventoryCapacityAndFinders()
{
    Inventory inv(2);
    CHECK(inv.empty());
    CHECK(!inv.isFull());

    CHECK(inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "", 10, 10)));
    CHECK(inv.addItem(std::make_unique<PotionItem>("potion_hp", "Potion", "", 30, 50)));
    CHECK(inv.isFull());
    CHECK(!inv.addItem(std::make_unique<FoodItem>("food_extra", "Extra", "", 10, 10)));
    CHECK_EQ(inv.findItemById("potion_hp")->name(), std::string("Potion"));
    CHECK(inv.itemAt(5) == nullptr);

    Character dummy("Dummy", 100, 50);
    CHECK(!inv.useItem(5, dummy));
    CHECK(!inv.removeItem(5));

    auto taken = inv.takeItem(1);
    CHECK(taken != nullptr);
    CHECK_EQ(inv.size(), static_cast<std::size_t>(1));
}

void testInventoryRemoveById()
{
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "", 10, 10));
    CHECK_EQ(inv.countItem("food_bread"), 1);
    CHECK(inv.removeItemById("food_bread", 1));
    CHECK_EQ(inv.countItem("food_bread"), 0);
    CHECK(inv.empty());
}

// ---- Shop ----

void testShopBuyFailsWithoutGold()
{
    Shop shop;
    shop.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "", 10, 10));
    Character buyer("Poor", 100, 50);
    Inventory inv;
    CHECK(!shop.buy(0, buyer, inv));
    CHECK(inv.empty());
}

void testShopSellItemReturnsGold()
{
    Shop shop;
    Character seller("Seller", 100, 50);
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "", 10, 10));
    int goldBefore = seller.gold();
    CHECK(shop.sell(0, seller, inv));
    CHECK(seller.gold() > goldBefore);
    CHECK(inv.empty());
}

// ---- Enemy ----

void testEnemyCloneIsIndependent()
{
    Slime slime;
    slime.takeDamage(10);
    auto cloned = slime.clone();
    CHECK_EQ(cloned->hp(), slime.maxHp());
    CHECK(cloned->hp() != slime.hp());
}

void testEnemyScaleToLevel()
{
    Slime slime;
    int baseStr = slime.strength();
    slime.scaleToLevel(5);
    CHECK(slime.strength() > baseStr);
    CHECK_EQ(slime.hp(), slime.maxHp());
}

void testEnemyChooseSkillFollowsPattern()
{
    Slime slime;
    CHECK(slime.chooseSkill(0) != nullptr);          // skill_slash
    CHECK(slime.chooseSkill(1) == nullptr);          // "normal"
    CHECK(slime.chooseSkill(2) != nullptr);          // skill_slash again
}

void testEnemyDropPersonaIds()
{
    Slime slime;
    CHECK(slime.dropPersonaIds().empty());
    slime.addDropPersonaId("persona_pixie");
    CHECK_EQ(slime.dropPersonaIds().size(), static_cast<std::size_t>(1));
}

// ---- BattleSystem ----

void testBattleSkillUsesSpAndDealsDamage()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 5, 20, 100));
    Slime slime;
    slime.scaleToLevel(1);

    BattleSystem battle;
    battle.startBattle(hero, slime);

    auto skill = std::make_shared<Skill>("skill_agi", "Agi", Element::Fire, 8, 6, SkillCostType::SP);
    int spBefore = hero.sp();
    int hpBefore = slime.hp();

    if (battle.isPlayerTurn())
    {
        CHECK(battle.playerUseSkill(0, *skill));
        CHECK(hero.sp() < spBefore);
        CHECK(slime.hp() < hpBefore);
    }
}

void testBattleItemIsFreeAction()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 20, 20, 100));
    Slime slime;
    BattleSystem battle;
    battle.startBattle(hero, slime);

    auto potion = std::make_unique<PotionItem>("potion_hp", "Potion", "", 30, 50);
    hero.takeDamage(30);
    int hpBefore = hero.hp();

    if (battle.isPlayerTurn())
    {
        CHECK(battle.playerUseItem(std::shared_ptr<Item>(std::move(potion))));
        CHECK(hero.hp() > hpBefore);
        CHECK(battle.isPlayerTurn()); // free action: still player's turn
    }
}

void testBattleSwitchPersonaConsumesTurn()
{
    Character hero("Hero", 100, 50);
    auto p1 = makeBasicPersona("p1", 20, 20, 100);
    auto p2 = makeBasicPersona("p2", 25, 25, 100);
    hero.addPersona(p1);
    hero.addPersona(p2);
    hero.setPersona(p1);

    Slime slime;
    BattleSystem battle;
    battle.startBattle(hero, slime);

    if (battle.isPlayerTurn())
    {
        CHECK(battle.playerSwitchPersona(p2));
        CHECK_EQ(hero.currentPersona()->id(), std::string("p2"));
        CHECK(!battle.isPlayerTurn()); // consumes turn
    }
}

void testBattleWeakAffinityDealsMoreDamage()
{
    Character hero("Hero", 100, 50);
    hero.setPersona(makeBasicPersona("p", 5, 15, 100));

    Slime slime;
    slime.setAffinity(Element::Fire, Affinity::Weak);
    BattleSystem battle;
    battle.startBattle(hero, slime);

    auto skill = std::make_shared<Skill>("skill_agi", "Agi", Element::Fire, 10, 5, SkillCostType::SP);
    int hpBefore = slime.hp();

    if (battle.isPlayerTurn())
    {
        battle.playerUseSkill(0, *skill);
        int damage = hpBefore - slime.hp();
        CHECK(damage > 10); // base ~16 * 1.5 weak multiplier, so should exceed 10
    }
}

// ---- Quest / QuestManager ----

void testQuestKillCompletion()
{
    Quest q("quest_test", "Test", "Defeat 2 shadows", "kill:2", 50, 20);
    q.setType(QuestType::Kill);
    q.setTargetCount(2);
    CHECK(!q.checkCompletion(0));
    CHECK(!q.checkCompletion(1));
    CHECK(q.checkCompletion(2));
}

void testQuestManagerAddKillProgress()
{
    QuestManager qm;
    Quest q("quest_test", "Test", "Defeat 2 shadows", "kill:2", 50, 20);
    q.setType(QuestType::Kill);
    q.setTargetCount(2);
    qm.addQuest(std::move(q));
    qm.acceptQuest("quest_test");

    qm.addKillProgress(1);
    CHECK(!qm.getQuest("quest_test")->isCompleted());
    qm.addKillProgress(1);
    CHECK(qm.getQuest("quest_test")->isCompleted());
}

void testQuestManagerRewardQuest()
{
    QuestManager qm;
    Quest q("quest_test", "Test", "", "kill:1", 100, 30);
    q.setType(QuestType::Kill);
    q.setTargetCount(1);
    qm.addQuest(std::move(q));
    qm.acceptQuest("quest_test");
    qm.getQuest("quest_test")->complete();

    CHECK(qm.rewardQuest("quest_test"));
    CHECK(qm.getQuest("quest_test")->isRewarded());
    CHECK(!qm.rewardQuest("quest_test")); // already rewarded
}

// ---- SocialLinkManager advanced ----

void testSocialLinkManagerSkillReward()
{
    SocialLinkManager mgr;
    SocialLink sl("sl_test", "Test", "Fool");
    auto skill = std::make_shared<Skill>("skill_reward", "Reward", Element::Fire, 10, 5, SkillCostType::SP);
    SocialLinkRankData data;
    data.reward.newSkill = skill;
    sl.setRankData(1, data);
    mgr.addLink(std::move(sl));

    mgr.addPoints("sl_test", 20);
    auto reward = mgr.currentSkillReward("sl_test");
    CHECK(reward != nullptr);
    CHECK_EQ(reward->id(), std::string("skill_reward"));
}

// ---- Entity / TileMap advanced ----

void testEntityMoveAndWorldBounds()
{
    PlayerEntity p({10, 20});
    p.move({5, -5});
    CHECK_EQ(p.position().x, 15.0f);
    CHECK_EQ(p.position().y, 15.0f);

    auto b = p.worldBounds();
    CHECK(b.width > 0);
    CHECK(b.height > 0);
}

void testTileMapEntityQueries()
{
    TileMap map(10, 10);
    map.addEntity(std::make_unique<PlayerEntity>(engine::Vec2{5, 5}));
    map.addEntity(std::make_unique<NpcEntity>(engine::Vec2{5, 5}, "sl_test"));

    engine::Rect area{0, 0, 20, 20};
    auto all = map.entitiesAt(area);
    CHECK_EQ(all.size(), static_cast<std::size_t>(2));

    Entity *npc = map.firstEntityAt(area, "npc");
    CHECK(npc != nullptr);
    CHECK_EQ(npc->type(), std::string("npc"));

    CHECK(map.removeEntity(npc));
    CHECK_EQ(map.entities().size(), static_cast<std::size_t>(1));
    map.clearEntities();
    CHECK(map.entities().empty());
}

int main()
{
    testCharacterGainExpTriggersLevelUp();
    testCharacterLevelUpSnapshotPopulated();
    testCharacterLevelUpIncreasesStats();
    testCharacterTakeDamageNoDefense();

    testFoodItemHealsCharacter();
    testEquipmentItemBoostsStrength();

    testInventoryAddItemIncreasesSize();
    testInventoryUseItemAppliesEffectAndRemovesIt();

    testShopBuyItemTransfersItemAndConsumesGold();
    testBattleStrongPlayerDefeatsSlime();

    testSocialLinkProgression();
    testSocialLinkAddPointsReturnsRanksGained();
    testSocialLinkMaxRankCapsAtTen();
    testSocialLinkRankDataDialogueAndReward();
    testSocialLinkManagerAddPointsNotifiesRankUp();
    testSocialLinkManagerDialogueForCurrentRank();
    testPersonaLevelsFromSocialLink();

    testEntityIntersection();
    testTileMapWalkability();
    testPersonaSkillUnlockOnLevelUp();

    testCharacterPersonaOwnership();
    testCharacterEquipmentBonuses();
    testCharacterSpAndGold();
    testCharacterHpCannotGoNegative();

    testSkillCanUseAndApplyCost();
    testSkillClonePreservesFields();

    testItemQuantityAndClone();
    testMiscItemAndPersonaItemNoOp();
    testInventoryCapacityAndFinders();
    testInventoryRemoveById();

    testShopBuyFailsWithoutGold();
    testShopSellItemReturnsGold();

    testEnemyCloneIsIndependent();
    testEnemyScaleToLevel();
    testEnemyChooseSkillFollowsPattern();
    testEnemyDropPersonaIds();

    testBattleSkillUsesSpAndDealsDamage();
    testBattleItemIsFreeAction();
    testBattleSwitchPersonaConsumesTurn();
    testBattleWeakAffinityDealsMoreDamage();

    testQuestKillCompletion();
    testQuestManagerAddKillProgress();
    testQuestManagerRewardQuest();

    testSocialLinkManagerSkillReward();

    testEntityMoveAndWorldBounds();
    testTileMapEntityQueries();

    std::cout << "\n==== CampusRPG core tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
