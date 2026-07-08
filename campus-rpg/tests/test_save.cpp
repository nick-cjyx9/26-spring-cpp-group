// CampusRPG save/persistence tests — exercises the SQLite-backed multi-slot
// SaveRepository. Links the data layer (via CampusRPGAppLib) + SQLite3.

#include "DatabaseManager.h"
#include "SaveRepository.h"
#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"
#include "Skill.h"
#include "SocialLink.h"
#include "SocialLinkManager.h"
#include "Quest.h"
#include "QuestManager.h"
#include "GameManager.h"

#include <cstdio>
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

    // RAII helper: open a fresh temp DB for a test, remove the file on teardown.
    struct TempDatabase
    {
        std::string path;
        explicit TempDatabase(const std::string &name) : path(name)
        {
            std::remove(path.c_str());
            DatabaseManager::instance().closeDatabase();
            DatabaseManager::instance().initDatabase(path);
        }
        ~TempDatabase()
        {
            DatabaseManager::instance().closeDatabase();
            std::remove(path.c_str());
        }
    };
} // namespace

void testEmptySlotsReportedAsEmpty()
{
    TempDatabase db("test_save_empty.db");
    SaveRepository repo;
    CHECK(!repo.slotExists(1));
    CHECK(!repo.slotExists(2));
    CHECK(!repo.slotExists(3));

    auto slots = repo.listSlots(3);
    CHECK_EQ(slots.size(), static_cast<std::size_t>(3));
    for (const auto &s : slots)
        CHECK(!s.exists);

    SaveSlotInfo info = repo.slotInfo(2);
    CHECK_EQ(info.slotId, 2);
    CHECK(!info.exists);
}

void testSaveAndLoadCharacterFullState()
{
    TempDatabase db("test_save_charstate.db");
    SaveRepository repo;

    Character hero("Hero", 100, 50);
    hero.gainExp(100); // level up to 2
    hero.addGold(250);
    hero.takeDamage(30); // hp < maxHp
    hero.consumeSp(10);

    int expectedLevel = hero.level();
    int expectedHp = hero.hp();
    int expectedSp = hero.sp();
    int expectedGold = hero.gold();

    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> personas;

    CHECK(repo.saveAll(1, hero, inv, personas, slm, qm));
    CHECK(repo.slotExists(1));

    Character loaded;
    Inventory loadedInv;
    std::vector<std::shared_ptr<Persona>> loadedPersonas;
    SocialLinkManager loadedSlm;
    QuestManager loadedQm;
    CHECK(repo.loadAll(1, loaded, loadedInv, loadedPersonas, loadedSlm, loadedQm));

    CHECK_EQ(loaded.name(), std::string("Hero"));
    CHECK_EQ(loaded.level(), expectedLevel);
    CHECK_EQ(loaded.hp(), expectedHp);
    CHECK_EQ(loaded.sp(), expectedSp);
    CHECK_EQ(loaded.gold(), expectedGold);
    CHECK_EQ(loaded.maxHp(), hero.maxHp());
    // base stats now come from Persona, not persisted on Character

    auto info = repo.slotInfo(1);
    CHECK(info.exists);
    CHECK_EQ(info.characterName, std::string("Hero"));
    CHECK_EQ(info.level, expectedLevel);
}

void testInventoryPersistsAcrossLoad()
{
    TempDatabase db("test_save_inv.db");
    SaveRepository repo;

    Character hero("Hero", 100, 50);
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "tasty", 10, 20));
    inv.addItem(std::make_unique<PotionItem>("potion_hp", "HP Potion", "heals", 30, 50));
    inv.addItem(std::make_unique<EquipmentItem>("eq_sword", "Sword", "sharp", 50, 5, 0, 0, EquipmentSlot::Weapon));
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> personas;

    CHECK(repo.saveAll(1, hero, inv, personas, slm, qm));

    Character loadedC;
    Inventory loadedInv;
    std::vector<std::shared_ptr<Persona>> loadedP;
    SocialLinkManager loadedS;
    QuestManager loadedQ;
    CHECK(repo.loadAll(1, loadedC, loadedInv, loadedP, loadedS, loadedQ));
    CHECK_EQ(loadedInv.size(), static_cast<std::size_t>(3));
    CHECK_EQ(std::string(loadedInv.itemAt(0)->name()), std::string("Bread"));
    CHECK_EQ(loadedInv.itemAt(2)->type(), ItemType::Equipment);
}

void testQuestsAndSocialLinksPersist()
{
    TempDatabase db("test_save_quests.db");
    SaveRepository repo;

    Character hero("Hero", 100, 50);
    Inventory inv;
    SocialLinkManager slm;
    slm.addLink(SocialLink("sl_yosuke", "Yosuke", "Magician"));
    slm.getLink("sl_yosuke")->addPoints(40); // should rank up
    QuestManager qm;
    qm.addQuest(Quest("quest_first", "First Steps", "", "kill:1", 20, 30));
    qm.acceptQuest("quest_first");
    std::vector<std::shared_ptr<Persona>> personas;

    CHECK(repo.saveAll(1, hero, inv, personas, slm, qm));

    Character lc;
    Inventory li;
    std::vector<std::shared_ptr<Persona>> lp;
    SocialLinkManager ls;
    QuestManager lq;
    // Definitions must exist before loading progress.
    ls.addLink(SocialLink("sl_yosuke", "Yosuke", "Magician"));
    lq.addQuest(Quest("quest_first", "First Steps", "", "kill:1", 20, 30));
    CHECK(repo.loadAll(1, lc, li, lp, ls, lq));

    CHECK(ls.getLink("sl_yosuke")->rank() > 0);
    CHECK(lq.getQuest("quest_first")->isAccepted());
}

void testMultipleSlotsAreIndependent()
{
    TempDatabase db("test_save_multi.db");
    SaveRepository repo;

    Character a("Alice", 100, 50);
    a.addGold(100);
    Character b("Bob", 120, 60);
    b.addGold(500);

    Inventory invA, invB;
    SocialLinkManager slmA, slmB;
    QuestManager qmA, qmB;
    std::vector<std::shared_ptr<Persona>> pa, pb;

    CHECK(repo.saveAll(1, a, invA, pa, slmA, qmA));
    CHECK(repo.saveAll(2, b, invB, pb, slmB, qmB));

    auto slots = repo.listSlots(3);
    CHECK_EQ(slots.size(), static_cast<std::size_t>(3));
    CHECK(slots[0].exists);
    CHECK_EQ(slots[0].characterName, std::string("Alice"));
    CHECK(slots[1].exists);
    CHECK_EQ(slots[1].characterName, std::string("Bob"));
    CHECK(!slots[2].exists);

    Character loaded;
    Inventory li;
    std::vector<std::shared_ptr<Persona>> lp;
    SocialLinkManager ls;
    QuestManager lq;
    CHECK(repo.loadAll(2, loaded, li, lp, ls, lq));
    CHECK_EQ(loaded.name(), std::string("Bob"));
    CHECK_EQ(loaded.gold(), 500);

    // Slot 1 must be unaffected by the slot 2 load.
    Character l1;
    Inventory i1;
    std::vector<std::shared_ptr<Persona>> p1;
    SocialLinkManager s1;
    QuestManager q1;
    CHECK(repo.loadAll(1, l1, i1, p1, s1, q1));
    CHECK_EQ(l1.name(), std::string("Alice"));
    CHECK_EQ(l1.gold(), 100);
}

void testOverwriteSlotReplacesPreviousData()
{
    TempDatabase db("test_save_overwrite.db");
    SaveRepository repo;

    Character first("First", 100, 50);
    first.addGold(50);
    Character second("Second", 100, 50);
    second.addGold(999);

    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> ps;

    CHECK(repo.saveAll(1, first, inv, ps, slm, qm));
    CHECK(repo.saveAll(1, second, inv, ps, slm, qm));

    Character loaded;
    Inventory li;
    std::vector<std::shared_ptr<Persona>> lp;
    SocialLinkManager ls;
    QuestManager lq;
    CHECK(repo.loadAll(1, loaded, li, lp, ls, lq));
    CHECK_EQ(loaded.name(), std::string("Second"));
    CHECK_EQ(loaded.gold(), 999);
}

void testDeleteSlotRemovesData()
{
    TempDatabase db("test_save_delete.db");
    SaveRepository repo;

    Character hero("Hero", 100, 50);
    Inventory inv;
    inv.addItem(std::make_unique<FoodItem>("food_bread", "Bread", "", 10, 20));
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> ps;

    CHECK(repo.saveAll(1, hero, inv, ps, slm, qm));
    CHECK(repo.slotExists(1));

    CHECK(repo.deleteSlot(1));
    CHECK(!repo.slotExists(1));
    CHECK(!repo.slotInfo(1).exists);

    // Loading a deleted slot must fail (not corrupt state).
    Character lc;
    Inventory li;
    std::vector<std::shared_ptr<Persona>> lp;
    SocialLinkManager ls;
    QuestManager lq;
    CHECK(!repo.loadAll(1, lc, li, lp, ls, lq));
}

void testLoadNonexistentSlotFailsCleanly()
{
    TempDatabase db("test_save_noexist.db");
    SaveRepository repo;

    Character hero("Hero", 100, 50);
    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> ps;

    CHECK(!repo.loadAll(1, hero, inv, ps, slm, qm));
    CHECK(!repo.slotExists(1));
}

void testCurrentPersonaIdPersisted()
{
    TempDatabase db("test_save_persona.db");
    SaveRepository repo;

    auto izanagi = std::make_shared<Persona>("persona_izanagi", "Izanagi", "Fool", 2, 6, 5, 5);
    auto pixie = std::make_shared<Persona>("persona_pixie", "Pixie", "Magician", 2, 4, 6, 5);

    Character hero("Hero", 100, 50);
    hero.setPersona(pixie); // equip the non-default persona

    std::vector<std::shared_ptr<Persona>> personas = {izanagi, pixie};
    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;

    CHECK(repo.saveAll(1, hero, inv, personas, slm, qm));
    CHECK_EQ(repo.currentPersonaId(1), std::string("persona_pixie"));

    // Slot 2 with no persona equipped.
    Character noPersona("NoPersona", 100, 50);
    CHECK(repo.saveAll(2, noPersona, inv, personas, slm, qm));
    CHECK_EQ(repo.currentPersonaId(2), std::string(""));
}

void testListAllSlotsIsDynamic()
{
    TempDatabase db("test_save_listall.db");
    SaveRepository repo;

    // Start empty.
    CHECK(repo.listAllSlots().empty());

    Character a("Alice", 100, 50);
    Character b("Bob", 100, 50);
    Character c("Cara", 100, 50);
    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> ps;

    CHECK(repo.saveAll(1, a, inv, ps, slm, qm));
    CHECK(repo.saveAll(2, b, inv, ps, slm, qm));
    CHECK(repo.saveAll(3, c, inv, ps, slm, qm));

    auto all = repo.listAllSlots();
    CHECK_EQ(all.size(), static_cast<std::size_t>(3));
    CHECK_EQ(all[0].characterName, std::string("Alice"));
    CHECK_EQ(all[1].characterName, std::string("Bob"));
    CHECK_EQ(all[2].characterName, std::string("Cara"));

    // Deleting slot 2 leaves a gap; listAllSlots reflects the remaining two.
    CHECK(repo.deleteSlot(2));
    all = repo.listAllSlots();
    CHECK_EQ(all.size(), static_cast<std::size_t>(2));
    CHECK_EQ(all[0].slotId, 1);
    CHECK_EQ(all[1].slotId, 3);
}

void testNextSlotIdIsMaxPlusOne()
{
    TempDatabase db("test_save_nextid.db");
    SaveRepository repo;

    CHECK_EQ(repo.nextSlotId(), 1); // no saves yet

    Character a("Alice", 100, 50);
    Inventory inv;
    SocialLinkManager slm;
    QuestManager qm;
    std::vector<std::shared_ptr<Persona>> ps;
    CHECK(repo.saveAll(1, a, inv, ps, slm, qm));
    CHECK_EQ(repo.nextSlotId(), 2);

    CHECK(repo.saveAll(4, a, inv, ps, slm, qm)); // create a gap (slots 1 and 4)
    CHECK_EQ(repo.nextSlotId(), 5); // max + 1, gap preserved

    repo.deleteSlot(4);
    CHECK_EQ(repo.nextSlotId(), 2);
}

void testGameManagerLoadRestoresPersonaSkills()
{
    TempDatabase db("test_save_skills.db");

    // createNewSave seeds default Personas (Izanagi/Pixie/Orpheus WITH skills) and
    // saves them, then binds the session to the new slot.
    CHECK(GameManager::instance().createNewSave("Hero"));
    int slot = GameManager::instance().currentSlotId();
    CHECK(slot >= 1);

    // The freshly-created game's starter persona must have its skills.
    Persona *starter = GameManager::instance().character().currentPersona();
    CHECK(starter != nullptr);
    CHECK(!starter->skills().empty());
    std::vector<std::string> starterSkillIds;
    for (const auto &s : starter->skills())
        if (s)
            starterSkillIds.push_back(s->id());

    // Reload the slot: loadFromSlot re-seeds defaults, snapshots their skills,
    // loads the save (Personas reconstructed without skills), then re-applies
    // the snapshotted skills. The equipped persona must regain its skills.
    CHECK(GameManager::instance().loadFromSlot(slot));
    Persona *loaded = GameManager::instance().character().currentPersona();
    CHECK(loaded != nullptr);
    for (const auto &id : starterSkillIds)
    {
        CHECK(loaded->findSkill(id) != nullptr);
    }
}

int main()
{
    testEmptySlotsReportedAsEmpty();
    testSaveAndLoadCharacterFullState();
    testInventoryPersistsAcrossLoad();
    testQuestsAndSocialLinksPersist();
    testMultipleSlotsAreIndependent();
    testOverwriteSlotReplacesPreviousData();
    testDeleteSlotRemovesData();
    testLoadNonexistentSlotFailsCleanly();
    testCurrentPersonaIdPersisted();
    testListAllSlotsIsDynamic();
    testNextSlotIdIsMaxPlusOne();
    testGameManagerLoadRestoresPersonaSkills();

    std::cout << "\n==== CampusRPG save/persistence tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
