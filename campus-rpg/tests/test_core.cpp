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
    testEntityIntersection();
    testTileMapWalkability();

    std::cout << "\n==== CampusRPG core tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
