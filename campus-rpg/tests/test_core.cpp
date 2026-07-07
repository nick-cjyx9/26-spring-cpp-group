// CampusRPG core unit tests — pure C++ lightweight runner (no SFML/SQLite dependency).

#include "Character.h"
#include "Inventory.h"
#include "Item.h"
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

void testCharacterGainExpTriggersLevelUp()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    CHECK_EQ(hero.level(), 1);
    hero.gainExp(100);
    CHECK_EQ(hero.level(), 2);
}

void testCharacterLevelUpIncreasesStats()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    int initialMaxHp = hero.maxHp();
    hero.gainExp(100);
    CHECK(hero.maxHp() > initialMaxHp);
}

void testCharacterTakeDamageAppliesDefense()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    hero.takeDamage(50);
    CHECK(hero.hp() > 0);
    CHECK(hero.hp() < 100);
}

void testFoodItemHealsCharacter()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    hero.takeDamage(50);
    FoodItem bread("food_bread", "Bread", "Tasty bread", 10, 30);
    bread.use(hero);
    CHECK(hero.hp() > 50);
}

void testEquipmentItemBoostsAttack()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    int baseAttack = hero.attack();
    EquipmentItem sword("eq_sword", "Sword", "Sharp sword", 50, 5, 0);
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
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
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
    Character buyer("Buyer", 100, 50, 10, 10, 10, 10, 10);
    buyer.addGold(100);
    Inventory inv;
    CHECK(shop.buy(0, buyer, inv));
    CHECK_EQ(inv.size(), static_cast<std::size_t>(1));
    CHECK_EQ(buyer.gold(), 90);
}

void testBattleStrongPlayerDefeatsSlime()
{
    Character hero("Hero", 100, 50, 20, 20, 20, 20, 20);
    Slime slime;
    BattleSystem battle;
    battle.startBattle(hero, slime);

    int safety = 0;
    while (!battle.isOver() && safety < 100)
    {
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
    link.setRankData(1, {"hello rank 1", {.hasStatBonus = true, .stat = PersonaStat::Strength, .statBonus = 2}});
    CHECK_EQ(link.currentDialogue(), "hello rank 0");
    link.addPoints(20); // -> rank 1
    CHECK_EQ(link.currentDialogue(), "hello rank 1");
    const SocialLinkReward *r = link.currentReward();
    CHECK(r != nullptr);
    CHECK(r->hasStatBonus);
    CHECK_EQ(r->statBonus, 2);
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

void testSocialLinkManagerArcanaBonusAggregation()
{
    SocialLinkManager mgr;
    // Two Magician links contributing Strength bonus at their current rank.
    SocialLink a("sl_a", "A", "Magician");
    a.setRankData(0, {"a0"});
    a.setRankData(1, {"a1", {.hasStatBonus = true, .stat = PersonaStat::Strength, .statBonus = 1}});
    SocialLink b("sl_b", "B", "Magician");
    b.setRankData(0, {"b0"});
    b.setRankData(1, {"b1", {.hasStatBonus = true, .stat = PersonaStat::Strength, .statBonus = 2}});
    // A Chariot link that should NOT count toward Magician.
    SocialLink c("sl_c", "C", "Chariot");
    c.setRankData(0, {"c0"});
    c.setRankData(1, {"c1", {.hasStatBonus = true, .stat = PersonaStat::Strength, .statBonus = 5}});
    mgr.addLink(std::move(a));
    mgr.addLink(std::move(b));
    mgr.addLink(std::move(c));

    // At rank 0: no rewards yet.
    CHECK_EQ(mgr.arcanaStatBonus("Magician", "Strength"), 0);
    // Rank up the two Magician links.
    mgr.addPoints("sl_a", 20);
    mgr.addPoints("sl_b", 20);
    CHECK_EQ(mgr.arcanaStatBonus("Magician", "Strength"), 3); // 1 + 2
    // Chariot bonus is separate.
    mgr.addPoints("sl_c", 20);
    CHECK_EQ(mgr.arcanaStatBonus("Chariot", "Strength"), 5);
    CHECK_EQ(mgr.arcanaStatBonus("Magician", "Agility"), 0); // wrong stat
}

void testCharacterAppliesSocialLinkBonus()
{
    Character hero("Hero", 100, 50, 10, 10, 10, 10, 10);
    int baseAtk = hero.attack();
    hero.applySocialLinkBonus(PersonaStat::Strength, 3);
    CHECK_EQ(hero.attack(), baseAtk + 3);
    hero.clearSocialLinkBonuses();
    CHECK_EQ(hero.attack(), baseAtk);
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

int main()
{
    testCharacterGainExpTriggersLevelUp();
    testCharacterLevelUpIncreasesStats();
    testCharacterTakeDamageAppliesDefense();

    testFoodItemHealsCharacter();
    testEquipmentItemBoostsAttack();

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
    testSocialLinkManagerArcanaBonusAggregation();
    testCharacterAppliesSocialLinkBonus();
    testEntityIntersection();
    testTileMapWalkability();

    std::cout << "\n==== CampusRPG core tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
