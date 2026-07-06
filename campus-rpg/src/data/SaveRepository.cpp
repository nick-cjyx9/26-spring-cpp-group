#include "SaveRepository.h"

#include "Character.h"
#include "DatabaseManager.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"
#include "Quest.h"
#include "QuestManager.h"
#include "Skill.h"
#include "SocialLink.h"
#include "SocialLinkManager.h"

#include <sqlite3.h>

#include <sstream>

namespace
{
    std::string extraDataFromItem(const Item &item)
    {
        std::ostringstream oss;
        switch (item.type())
        {
        case ItemType::Food:
        case ItemType::Potion:
            oss << "healAmount=" << static_cast<const FoodItem &>(item).healAmount();
            break;
        case ItemType::SpRecovery:
            oss << "spAmount=" << static_cast<const SpItem &>(item).spAmount();
            break;
        case ItemType::Equipment:
        {
            const auto &eq = static_cast<const EquipmentItem &>(item);
            oss << "attackBonus=" << eq.attackBonus()
                << ",defenseBonus=" << eq.defenseBonus()
                << ",speedBonus=" << eq.speedBonus();
            break;
        }
        case ItemType::Persona:
            oss << "personaId=" << static_cast<const PersonaItem &>(item).personaId();
            break;
        }
        return oss.str();
    }

    std::unique_ptr<Item> itemFromRecord(const char *itemId, const char *name, const char *description,
                                         int value, const char *typeStr, const char *extraData)
    {
        std::string extra = extraData ? extraData : "";
        auto getInt = [&extra](const std::string &key) -> int
        {
            size_t pos = extra.find(key + "=");
            if (pos == std::string::npos)
                return 0;
            pos += key.length() + 1;
            size_t end = extra.find(",", pos);
            std::string val = extra.substr(pos, end - pos);
            return std::stoi(val);
        };

        std::string id = itemId ? itemId : "";
        std::string n = name ? name : "";
        std::string desc = description ? description : "";

        if (std::string(typeStr) == "Food")
            return std::make_unique<FoodItem>(id, n, desc, value, getInt("healAmount"));
        if (std::string(typeStr) == "Potion")
            return std::make_unique<PotionItem>(id, n, desc, value, getInt("healAmount"));
        if (std::string(typeStr) == "SP")
            return std::make_unique<SpItem>(id, n, desc, value, getInt("spAmount"));
        if (std::string(typeStr) == "Equipment")
            return std::make_unique<EquipmentItem>(id, n, desc, value,
                                                   getInt("attackBonus"), getInt("defenseBonus"),
                                                   getInt("speedBonus"));
        if (std::string(typeStr) == "Persona")
        {
            size_t pos = extra.find("personaId=");
            std::string pid;
            if (pos != std::string::npos)
            {
                pos += 10;
                size_t end = extra.find(",", pos);
                pid = extra.substr(pos, end - pos);
            }
            return std::make_unique<PersonaItem>(id, n, desc, value, pid);
        }
        return nullptr;
    }

    std::string affinitiesToString(const Persona &p)
    {
        std::ostringstream oss;
        for (int i = 0; i < static_cast<int>(Element::Count); ++i)
        {
            Element e = static_cast<Element>(i);
            oss << static_cast<int>(e) << ":" << static_cast<int>(p.affinity(e));
            if (i + 1 < static_cast<int>(Element::Count))
                oss << ";";
        }
        return oss.str();
    }

    void applyAffinitiesFromString(Persona &p, const std::string &str)
    {
        std::istringstream iss(str);
        std::string pair;
        while (std::getline(iss, pair, ';'))
        {
            size_t sep = pair.find(':');
            if (sep == std::string::npos)
                continue;
            int e = std::stoi(pair.substr(0, sep));
            int a = std::stoi(pair.substr(sep + 1));
            p.setAffinity(static_cast<Element>(e), static_cast<Affinity>(a));
        }
    }

    std::string skillsToString(const Persona &p)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < p.skills().size(); ++i)
        {
            if (p.skills()[i])
            {
                oss << p.skills()[i]->id();
                if (i + 1 < p.skills().size())
                    oss << ",";
            }
        }
        return oss.str();
    }
} // namespace

bool SaveRepository::saveCharacter(const Character &character)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = R"(
        REPLACE INTO character (id, name, level, hp, max_hp, sp, max_sp, exp, exp_to_next, gold,
                                pos_x, pos_y, is_night, current_persona_id)
        VALUES (1, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, 0, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, character.name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, character.level());
    sqlite3_bind_int(stmt, 3, character.hp());
    sqlite3_bind_int(stmt, 4, character.maxHp());
    sqlite3_bind_int(stmt, 5, character.sp());
    sqlite3_bind_int(stmt, 6, character.maxSp());
    sqlite3_bind_int(stmt, 7, character.exp());
    sqlite3_bind_int(stmt, 8, character.expToNextLevel());
    sqlite3_bind_int(stmt, 9, character.gold());

    std::string personaId = character.currentPersona() ? character.currentPersona()->id() : "";
    sqlite3_bind_text(stmt, 10, personaId.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SaveRepository::loadCharacter(Character &character)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT * FROM character WHERE id = 1";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        int maxHp = sqlite3_column_int(stmt, 4);
        int maxSp = sqlite3_column_int(stmt, 6);
        character = Character(name, maxHp, maxSp, 5, 5, 5, 5, 5);
        // Restore additional fields would require setters; simplified for now.
        ok = true;
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool SaveRepository::saveInventory(const Inventory &inventory)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_exec(db, "DELETE FROM inventory", nullptr, nullptr, nullptr);

    const char *sql = R"(
        INSERT INTO inventory (item_id, item_type, name, description, value, extra_data)
        VALUES (?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto &item : inventory.items())
    {
        if (!item)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, item->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, item->typeString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, item->name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, item->description().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, item->value());
        std::string extra = extraDataFromItem(*item);
        sqlite3_bind_text(stmt, 6, extra.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadInventory(Inventory &inventory)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    inventory.clear();
    const char *sql = "SELECT * FROM inventory";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto item = itemFromRecord(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)),
            sqlite3_column_int(stmt, 5),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6)));
        if (item)
            inventory.addItem(std::move(item));
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::savePersonas(const std::vector<std::shared_ptr<Persona>> &personas)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_exec(db, "DELETE FROM persona", nullptr, nullptr, nullptr);

    const char *sql = R"(
        INSERT INTO persona (id, name, arcana, level, st, ma, en, ag, lu, affinities, skills, owner)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto &p : personas)
    {
        if (!p)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, p->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, p->name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, p->arcana().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, p->level());
        sqlite3_bind_int(stmt, 5, p->strength());
        sqlite3_bind_int(stmt, 6, p->magic());
        sqlite3_bind_int(stmt, 7, p->endurance());
        sqlite3_bind_int(stmt, 8, p->agility());
        sqlite3_bind_int(stmt, 9, p->luck());
        std::string affs = affinitiesToString(*p);
        sqlite3_bind_text(stmt, 10, affs.c_str(), -1, SQLITE_TRANSIENT);
        std::string skills = skillsToString(*p);
        sqlite3_bind_text(stmt, 11, skills.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 12, "player", -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadPersonas(std::vector<std::shared_ptr<Persona>> &personas,
                                  std::string &currentPersonaId)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    personas.clear();
    const char *sql = "SELECT * FROM persona WHERE owner = 'player'";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string arcana = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        int level = sqlite3_column_int(stmt, 3);
        int st = sqlite3_column_int(stmt, 4);
        int ma = sqlite3_column_int(stmt, 5);
        int en = sqlite3_column_int(stmt, 6);
        int ag = sqlite3_column_int(stmt, 7);
        int lu = sqlite3_column_int(stmt, 8);

        auto p = std::make_shared<Persona>(id, name, arcana, level, st, ma, en, ag, lu);
        const char *affs = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
        if (affs)
            applyAffinitiesFromString(*p, affs);
        personas.push_back(p);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::saveSocialLinks(const SocialLinkManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_exec(db, "DELETE FROM social_link", nullptr, nullptr, nullptr);

    const char *sql = "INSERT INTO social_link (id, rank, points) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto *link : manager.allLinks())
    {
        if (!link)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, link->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, link->rank());
        sqlite3_bind_int(stmt, 3, link->points());
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadSocialLinks(SocialLinkManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT * FROM social_link";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        SocialLink *link = manager.getLink(id);
        if (link)
        {
            int rank = sqlite3_column_int(stmt, 1);
            int points = sqlite3_column_int(stmt, 2);
            while (link->rank() < rank && link->rank() < 10)
                link->rankUp();
            link->addPoints(points);
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::saveQuests(const QuestManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_exec(db, "DELETE FROM quest_progress", nullptr, nullptr, nullptr);

    const char *sql = "INSERT INTO quest_progress (quest_id, accepted, completed, rewarded) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto *quest : manager.acceptedQuests())
    {
        if (!quest)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, quest->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, quest->isAccepted() ? 1 : 0);
        sqlite3_bind_int(stmt, 3, quest->isCompleted() ? 1 : 0);
        sqlite3_bind_int(stmt, 4, quest->isRewarded() ? 1 : 0);
        sqlite3_step(stmt);
    }
    for (const auto *quest : manager.completedQuests())
    {
        if (!quest)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, quest->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, quest->isAccepted() ? 1 : 0);
        sqlite3_bind_int(stmt, 3, quest->isCompleted() ? 1 : 0);
        sqlite3_bind_int(stmt, 4, quest->isRewarded() ? 1 : 0);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadQuests(QuestManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT * FROM quest_progress";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        Quest *q = manager.getQuest(id);
        if (!q)
            continue;
        if (sqlite3_column_int(stmt, 1))
            q->accept();
        if (sqlite3_column_int(stmt, 2))
            q->complete();
        if (sqlite3_column_int(stmt, 3))
            q->reward();
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::saveAll(const Character &character, const Inventory &inventory,
                             const std::vector<std::shared_ptr<Persona>> &personas,
                             const SocialLinkManager &socialLinks, const QuestManager &quests)
{
    if (!DatabaseManager::instance().isOpen())
        return false;
    return saveCharacter(character) && saveInventory(inventory) && savePersonas(personas) &&
           saveSocialLinks(socialLinks) && saveQuests(quests);
}

bool SaveRepository::loadAll(Character &character, Inventory &inventory,
                             std::vector<std::shared_ptr<Persona>> &personas,
                             SocialLinkManager &socialLinks, QuestManager &quests)
{
    if (!DatabaseManager::instance().isOpen())
        return false;
    std::string currentPersonaId;
    return loadCharacter(character) && loadInventory(inventory) &&
           loadPersonas(personas, currentPersonaId) && loadSocialLinks(socialLinks) &&
           loadQuests(quests);
}
