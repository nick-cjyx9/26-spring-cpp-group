// CampusRPG core unit tests — pure C++ lightweight runner (no Qt dependency).
//
// Rationale: the core layer is intentionally kept free of Qt/SQLite so that
// tests stay fast, portable and runnable in any environment (including
// headless CI and the MinGW command line). A tiny CHECK/CHECK_EQ macro
// runner is all we need; it prints PASS/FAIL lines and returns a non-zero
// exit code on any failure, which CTest picks up.
//
// Qt Test best practices are still followed where they apply to plain C++:
//  - descriptive, self-contained test functions (no cross-test dependencies);
//  - no assertions that abort the whole program (a failure is recorded and
//    the suite keeps running, mirroring QCOMPARE/QVERIFY semantics);
//  - classes under test are compiled into a static library (CampusRPGCore),
//    so the test target links the library instead of re-listing sources.

#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "Enemy.h"
#include "Shop.h"
#include "BattleSystem.h"

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

// ---- Character ----
void testCharacterGainExpTriggersLevelUp()
{
    Character hero("Hero", 100, 10, 5);
    CHECK_EQ(hero.level(), 1);

    hero.gainExp(100);
    CHECK_EQ(hero.level(), 2);
    CHECK_EQ(hero.exp(), 0);
}

void testCharacterLevelUpIncreasesStats()
{
    Character hero("Hero", 100, 10, 5);
    const int initialMaxHp = hero.maxHp();
    const int initialAttack = hero.attack();

    hero.gainExp(100);
    CHECK(hero.maxHp() > initialMaxHp);
    CHECK(hero.attack() > initialAttack);
}

void testCharacterTakeDamageAppliesDefense()
{
    Character hero("Hero", 100, 10, 5);
    hero.takeDamage(50); // 50 - 5 defense = 45
    CHECK_EQ(hero.hp(), 55);
    CHECK(hero.hp() > 0);
}

// ---- Item ----
void testFoodItemHealsCharacter()
{
    Character hero("Hero", 100, 10, 0);
    hero.takeDamage(50);
    CHECK_EQ(hero.hp(), 50);

    FoodItem bread("food_bread", "Bread", "Tasty bread", 10, 30);
    bread.use(hero);
    CHECK_EQ(hero.hp(), 80);
}

void testPotionItemHealsMoreThanFood()
{
    Character hero("Hero", 100, 10, 0);
    hero.takeDamage(80);

    PotionItem potion("potion_hp", "HP Potion", "Restores HP", 30, 50);
    potion.use(hero);
    CHECK_EQ(hero.hp(), 70);
}

void testEquipmentItemBoostsAttack()
{
    Character hero("Hero", 100, 10, 5);
    const int baseAttack = hero.attack();

    EquipmentItem sword("eq_sword", "Sword", "Sharp sword", 50, 5, 0);
    sword.use(hero);
    CHECK_EQ(hero.attack(), baseAttack + 5);
}

// ---- Inventory ----
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

    Character hero("Hero", 100, 10, 0);
    hero.takeDamage(60);
    CHECK_EQ(hero.hp(), 40);

    CHECK(inv.useItem(0, hero));
    CHECK_EQ(hero.hp(), 90);
    CHECK(inv.empty());
}

void testInventoryRemoveItemDropsIt()
{
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));
    CHECK(inv.removeItem(0));
    CHECK(inv.empty());
    CHECK(!inv.removeItem(0)); // out of range -> false
}

// ---- Shop ----
void testShopBuyItemTransfersItemAndConsumesGold()
{
    Shop shop;
    shop.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));

    Character buyer("Buyer", 100, 10, 5);
    buyer.addGold(100);
    Inventory inv;

    CHECK(shop.buy(0, buyer, inv));
    CHECK_EQ(inv.size(), static_cast<std::size_t>(1));
    CHECK_EQ(buyer.gold(), 90);
}

void testShopSellItemGrantsHalfValue()
{
    Shop shop;
    shop.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));

    Character buyer("Buyer", 100, 10, 5);
    buyer.addGold(100);
    Inventory inv;
    shop.buy(0, buyer, inv);

    CHECK(shop.sell(0, buyer, inv));
    CHECK(inv.empty());
    CHECK_EQ(buyer.gold(), 95); // 90 + floor(10/2)
}

void testShopBuyFailsWithoutEnoughGold()
{
    Shop shop;
    shop.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "Bread", 10, 15));

    Character buyer("Buyer", 100, 10, 5);
    Inventory inv;

    CHECK(!shop.buy(0, buyer, inv));
    CHECK(inv.empty());
}

// ---- Battle ----
void testBattleStrongPlayerDefeatsSlime()
{
    Character hero("Hero", 100, 50, 5);
    Slime slime;

    BattleSystem battle;
    battle.startBattle(hero, slime);

    int safety = 0;
    while (!battle.isOver() && safety < 100)
    {
        battle.playerAttack();
        if (!battle.isOver())
        {
            battle.enemyAttack();
        }
        ++safety;
    }

    CHECK(battle.isOver());
    CHECK(battle.playerWon());
}

int main()
{
    testCharacterGainExpTriggersLevelUp();
    testCharacterLevelUpIncreasesStats();
    testCharacterTakeDamageAppliesDefense();

    testFoodItemHealsCharacter();
    testPotionItemHealsMoreThanFood();
    testEquipmentItemBoostsAttack();

    testInventoryAddItemIncreasesSize();
    testInventoryUseItemAppliesEffectAndRemovesIt();
    testInventoryRemoveItemDropsIt();

    testShopBuyItemTransfersItemAndConsumesGold();
    testShopSellItemGrantsHalfValue();
    testShopBuyFailsWithoutEnoughGold();

    testBattleStrongPlayerDefeatsSlime();

    std::cout << "\n==== CampusRPG core tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
