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

#include <chrono>
#include <ctime>
#include <sstream>

namespace
{
    std::string extraDataFromItem(const Item &item)
    {
        std::ostringstream oss;
        if (!item.textureId().empty())
            oss << "textureId=" << item.textureId();
        switch (item.type())
        {
        case ItemType::Food:
        case ItemType::Potion:
            if (oss.tellp() > 0) oss << ",";
            oss << "healAmount=" << static_cast<const FoodItem &>(item).healAmount();
            break;
        case ItemType::SpRecovery:
            if (oss.tellp() > 0) oss << ",";
            oss << "spAmount=" << static_cast<const SpItem &>(item).spAmount();
            break;
        case ItemType::Equipment:
        {
            const auto &eq = static_cast<const EquipmentItem &>(item);
            if (oss.tellp() > 0) oss << ",";
            oss << "strengthBonus=" << eq.strengthBonus()
                << ",magicBonus=" << eq.magicBonus()
                << ",speedBonus=" << eq.speedBonus()
                << ",slot=" << static_cast<int>(eq.slot());
            break;
        }
        case ItemType::Persona:
            if (oss.tellp() > 0) oss << ",";
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
        auto getStr = [&extra](const std::string &key) -> std::string
        {
            size_t pos = extra.find(key + "=");
            if (pos == std::string::npos)
                return "";
            pos += key.length() + 1;
            size_t end = extra.find(",", pos);
            return extra.substr(pos, end - pos);
        };

        std::string id = itemId ? itemId : "";
        std::string n = name ? name : "";
        std::string desc = description ? description : "";
        std::string tex = getStr("textureId");

        if (std::string(typeStr) == "Food")
            return std::make_unique<FoodItem>(id, n, desc, value, getInt("healAmount"), tex);
        if (std::string(typeStr) == "Potion")
            return std::make_unique<PotionItem>(id, n, desc, value, getInt("healAmount"), tex);
        if (std::string(typeStr) == "SP")
            return std::make_unique<SpItem>(id, n, desc, value, getInt("spAmount"), tex);
        if (std::string(typeStr) == "Equipment")
            return std::make_unique<EquipmentItem>(id, n, desc, value,
                                                   getInt("strengthBonus"), getInt("magicBonus"),
                                                   getInt("speedBonus"),
                                                   static_cast<EquipmentSlot>(getInt("slot")),
                                                   tex);
        if (std::string(typeStr) == "Persona")
        {
            std::string pid = getStr("personaId");
            return std::make_unique<PersonaItem>(id, n, desc, value, pid, tex);
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
        bool first = true;
        for (const auto &s : p.skills())
        {
            if (s)
            {
                if (!first)
                    oss << ",";
                oss << s->id();
                first = false;
            }
        }
        return oss.str();
    }

    std::string currentTimeString()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#if defined(_WIN32)
        localtime_s(&tm, &t);
#else
        localtime_r(&t, &tm);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return std::string(buf);
    }
} // namespace

bool SaveRepository::saveCharacter_(int slotId, const Character &character)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = R"(
        REPLACE INTO character (slot_id, name, level, hp, max_hp, sp, max_sp, exp, exp_to_next, gold,
                                eq_str, eq_mag, eq_spd,
                                pos_x, pos_y, is_night, current_persona_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, 0, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);
    sqlite3_bind_text(stmt, 2, character.name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, character.level());
    sqlite3_bind_int(stmt, 4, character.hp());
    sqlite3_bind_int(stmt, 5, character.maxHp());
    sqlite3_bind_int(stmt, 6, character.sp());
    sqlite3_bind_int(stmt, 7, character.maxSp());
    sqlite3_bind_int(stmt, 8, character.exp());
    sqlite3_bind_int(stmt, 9, character.expToNextLevel());
    sqlite3_bind_int(stmt, 10, character.gold());
    sqlite3_bind_int(stmt, 11, character.equipmentStrengthBonus());
    sqlite3_bind_int(stmt, 12, character.equipmentMagicBonus());
    sqlite3_bind_int(stmt, 13, character.equipmentSpeedBonus());

    std::string personaId = character.currentPersona() ? character.currentPersona()->id() : "";
    sqlite3_bind_text(stmt, 14, personaId.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SaveRepository::loadCharacter_(int slotId, Character &character)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT name, level, hp, max_hp, sp, max_sp, exp, exp_to_next, gold, "
                      "eq_str, eq_mag, eq_spd, current_persona_id "
                      "FROM character WHERE slot_id = ?";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        int level = sqlite3_column_int(stmt, 1);
        int hp = sqlite3_column_int(stmt, 2);
        int maxHp = sqlite3_column_int(stmt, 3);
        int sp = sqlite3_column_int(stmt, 4);
        int maxSp = sqlite3_column_int(stmt, 5);
        int exp = sqlite3_column_int(stmt, 6);
        int expToNext = sqlite3_column_int(stmt, 7);
        int gold = sqlite3_column_int(stmt, 8);
        int eqStr = sqlite3_column_int(stmt, 9);
        int eqMag = sqlite3_column_int(stmt, 10);
        int eqSpd = sqlite3_column_int(stmt, 11);
        const char *personaIdText = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 12));

        // Reconstruct the character with base vitals, then restore all fields.
        character = Character(name, maxHp, maxSp);
        character.setLevel(level);
        character.setHp(hp);
        character.setSp(sp);
        character.setExp(exp);
        character.setExpToNextLevel(expToNext);
        character.setGold(gold);
        character.setEquipmentBonuses(eqStr, eqMag, eqSpd);
        // current_persona_id is resolved by the caller (GameManager::loadFromSlot)
        // via findPersona(); store nothing here beyond what Character exposes.
        (void)personaIdText;
        ok = true;
    }

    sqlite3_finalize(stmt);
    return ok;
}

bool SaveRepository::saveInventory_(int slotId, const Inventory &inventory)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM inventory WHERE slot_id = ?", -1, &delStmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(delStmt, 1, slotId);
    sqlite3_step(delStmt);
    sqlite3_finalize(delStmt);

    const char *sql = R"(
        INSERT INTO inventory (slot_id, ord, item_id, item_type, name, description, value, extra_data, texture_id, quantity)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    int ord = 0;
    for (const auto &item : inventory.items())
    {
        if (!item)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, slotId);
        sqlite3_bind_int(stmt, 2, ord++);
        sqlite3_bind_text(stmt, 3, item->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, item->typeString().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, item->name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, item->description().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 7, item->value());
        std::string extra = extraDataFromItem(*item);
        sqlite3_bind_text(stmt, 8, extra.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 9, item->textureId().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 10, item->quantity());
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadInventory_(int slotId, Inventory &inventory)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    inventory.clear();
    const char *sql = "SELECT item_id, item_type, name, description, value, extra_data, texture_id, quantity "
                      "FROM inventory WHERE slot_id = ? ORDER BY ord";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto item = itemFromRecord(
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)),
            sqlite3_column_int(stmt, 4),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1)),
            reinterpret_cast<const char *>(sqlite3_column_text(stmt, 5)));
        if (item)
        {
            const char *texCol = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
            if (texCol && texCol[0] != '\0')
                item->setTextureId(texCol);
            int qty = sqlite3_column_int(stmt, 7);
            if (qty > 0)
                item->setQuantity(qty);
            inventory.addItem(std::move(item));
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::savePersonas_(int slotId, const std::vector<std::shared_ptr<Persona>> &personas)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM persona WHERE slot_id = ?", -1, &delStmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(delStmt, 1, slotId);
    sqlite3_step(delStmt);
    sqlite3_finalize(delStmt);

    const char *sql = R"(
        INSERT INTO persona (slot_id, id, name, arcana, level, strength, magic, speed, affinities, skills, owner)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto &p : personas)
    {
        if (!p)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, slotId);
        sqlite3_bind_text(stmt, 2, p->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, p->name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, p->arcana().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, p->level());
        sqlite3_bind_int(stmt, 6, p->strength());
        sqlite3_bind_int(stmt, 7, p->magic());
        sqlite3_bind_int(stmt, 8, p->speed());
        std::string affs = affinitiesToString(*p);
        sqlite3_bind_text(stmt, 9, affs.c_str(), -1, SQLITE_TRANSIENT);
        std::string skills = skillsToString(*p);
        sqlite3_bind_text(stmt, 10, skills.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 11, "player", -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadPersonas_(int slotId, std::vector<std::shared_ptr<Persona>> &personas,
                                   std::string &currentPersonaId)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    personas.clear();
    const char *sql = "SELECT id, name, arcana, level, strength, magic, speed, affinities, skills "
                      "FROM persona WHERE slot_id = ? AND owner = 'player'";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        std::string name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        std::string arcana = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        int level = sqlite3_column_int(stmt, 3);
        int st = sqlite3_column_int(stmt, 4);
        int ma = sqlite3_column_int(stmt, 5);
        int sp = sqlite3_column_int(stmt, 6);

        auto p = std::make_shared<Persona>(id, name, arcana, level, st, ma, sp);
        const char *affs = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 9));
        if (affs)
            applyAffinitiesFromString(*p, affs);
        // Skills are persisted as a comma-separated id list; reconstructing Skill
        // objects requires a shared skill registry (not yet available). The id list
        // is stored so future skill restoration can be added without a schema change.
        personas.push_back(p);
    }

    // current_persona_id lives on the character row; loadCharacter_ reads it.
    (void)currentPersonaId;

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::saveSocialLinks_(int slotId, const SocialLinkManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM social_link WHERE slot_id = ?", -1, &delStmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(delStmt, 1, slotId);
    sqlite3_step(delStmt);
    sqlite3_finalize(delStmt);

    const char *sql = "INSERT INTO social_link (slot_id, id, rank, points, name, portrait) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto *link : manager.allLinks())
    {
        if (!link)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, slotId);
        sqlite3_bind_text(stmt, 2, link->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, link->rank());
        sqlite3_bind_int(stmt, 4, link->points());
        sqlite3_bind_text(stmt, 5, link->name().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, link->portraitId().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadSocialLinks_(int slotId, SocialLinkManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT id, rank, points, name, portrait FROM social_link WHERE slot_id = ?";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string id = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        SocialLink *link = manager.getLink(id);
        if (link)
        {
            // Restore the persistent NPC identity (name/portrait) from the save
            // so the random pool stays stable across loads.
            if (const char *nm = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3)))
            {
                std::string name = nm;
                if (!name.empty())
                    link->setName(name);
            }
            if (const char *pt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4)))
            {
                std::string portrait = pt;
                if (!portrait.empty())
                    link->setPortraitId(portrait);
            }
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

bool SaveRepository::saveQuests_(int slotId, const QuestManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *delStmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM quest_progress WHERE slot_id = ?", -1, &delStmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(delStmt, 1, slotId);
    sqlite3_step(delStmt);
    sqlite3_finalize(delStmt);

    const char *sql = "INSERT INTO quest_progress (slot_id, quest_id, accepted, completed, rewarded) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    for (const auto *quest : manager.acceptedQuests())
    {
        if (!quest)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, slotId);
        sqlite3_bind_text(stmt, 2, quest->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, quest->isAccepted() ? 1 : 0);
        sqlite3_bind_int(stmt, 4, quest->isCompleted() ? 1 : 0);
        sqlite3_bind_int(stmt, 5, quest->isRewarded() ? 1 : 0);
        sqlite3_step(stmt);
    }
    for (const auto *quest : manager.completedQuests())
    {
        if (!quest)
            continue;
        sqlite3_reset(stmt);
        sqlite3_bind_int(stmt, 1, slotId);
        sqlite3_bind_text(stmt, 2, quest->id().c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 3, quest->isAccepted() ? 1 : 0);
        sqlite3_bind_int(stmt, 4, quest->isCompleted() ? 1 : 0);
        sqlite3_bind_int(stmt, 5, quest->isRewarded() ? 1 : 0);
        sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::loadQuests_(int slotId, QuestManager &manager)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "SELECT quest_id, accepted, completed, rewarded FROM quest_progress WHERE slot_id = ?";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);

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

bool SaveRepository::saveMeta_(int slotId, const Character &character)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    const char *sql = "REPLACE INTO save_meta (slot_id, character_name, level, updated_at) VALUES (?, ?, ?, ?)";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int(stmt, 1, slotId);
    sqlite3_bind_text(stmt, 2, character.name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, character.level());
    sqlite3_bind_text(stmt, 4, currentTimeString().c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool SaveRepository::deleteMeta_(int slotId)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM save_meta WHERE slot_id = ?", -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(stmt, 1, slotId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return true;
}

bool SaveRepository::saveAll(int slotId, const Character &character, const Inventory &inventory,
                             const std::vector<std::shared_ptr<Persona>> &personas,
                             const SocialLinkManager &socialLinks, const QuestManager &quests)
{
    if (!DatabaseManager::instance().isOpen())
        return false;

    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    // Wrap the whole slot write in a transaction so a partial failure leaves
    // the previous slot state intact.
    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);

    bool ok = saveCharacter_(slotId, character) &&
              saveInventory_(slotId, inventory) &&
              savePersonas_(slotId, personas) &&
              saveSocialLinks_(slotId, socialLinks) &&
              saveQuests_(slotId, quests) &&
              saveMeta_(slotId, character);

    if (ok)
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    else
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return ok;
}

bool SaveRepository::loadAll(int slotId, Character &character, Inventory &inventory,
                             std::vector<std::shared_ptr<Persona>> &personas,
                             SocialLinkManager &socialLinks, QuestManager &quests)
{
    if (!DatabaseManager::instance().isOpen())
        return false;

    if (!slotExists(slotId))
        return false;

    std::string currentPersonaId;
    return loadCharacter_(slotId, character) && loadInventory_(slotId, inventory) &&
           loadPersonas_(slotId, personas, currentPersonaId) && loadSocialLinks_(slotId, socialLinks) &&
           loadQuests_(slotId, quests);
}

bool SaveRepository::deleteSlot(int slotId)
{
    if (!DatabaseManager::instance().isOpen())
        return false;

    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);

    const char *tables[] = {"character", "persona", "inventory", "social_link", "quest_progress", "save_meta"};
    bool ok = true;
    for (const char *t : tables)
    {
        std::string sql = std::string("DELETE FROM ") + t + " WHERE slot_id = ?";
        sqlite3_stmt *stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            ok = false;
            break;
        }
        sqlite3_bind_int(stmt, 1, slotId);
        if (sqlite3_step(stmt) != SQLITE_DONE)
            ok = false;
        sqlite3_finalize(stmt);
        if (!ok)
            break;
    }

    if (ok)
        sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
    else
        sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr);
    return ok;
}

bool SaveRepository::slotExists(int slotId)
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return false;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT 1 FROM save_meta WHERE slot_id = ?", -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_int(stmt, 1, slotId);
    bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

SaveSlotInfo SaveRepository::slotInfo(int slotId)
{
    SaveSlotInfo info;
    info.slotId = slotId;

    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return info;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT character_name, level, updated_at FROM save_meta WHERE slot_id = ?",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return info;
    sqlite3_bind_int(stmt, 1, slotId);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        info.exists = true;
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (name)
            info.characterName = name;
        info.level = sqlite3_column_int(stmt, 1);
        const char *ts = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        if (ts)
            info.updatedAt = ts;
    }
    sqlite3_finalize(stmt);
    return info;
}

std::vector<SaveSlotInfo> SaveRepository::listSlots(int maxSlotId)
{
    std::vector<SaveSlotInfo> result;
    result.reserve(static_cast<size_t>(maxSlotId));
    for (int id = 1; id <= maxSlotId; ++id)
        result.push_back(slotInfo(id));
    return result;
}

std::vector<SaveSlotInfo> SaveRepository::listAllSlots()
{
    std::vector<SaveSlotInfo> result;
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return result;

    const char *sql = "SELECT slot_id, character_name, level, updated_at FROM save_meta ORDER BY slot_id";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        SaveSlotInfo info;
        info.slotId = sqlite3_column_int(stmt, 0);
        info.exists = true;
        const char *name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        if (name)
            info.characterName = name;
        info.level = sqlite3_column_int(stmt, 2);
        const char *ts = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        if (ts)
            info.updatedAt = ts;
        result.push_back(info);
    }
    sqlite3_finalize(stmt);
    return result;
}

int SaveRepository::nextSlotId()
{
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return 1;

    int next = 1;
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COALESCE(MAX(slot_id), 0) + 1 FROM save_meta",
                           -1, &stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(stmt) == SQLITE_ROW)
            next = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
    }
    return next;
}

std::string SaveRepository::currentPersonaId(int slotId)
{
    std::string id;
    sqlite3 *db = DatabaseManager::instance().database();
    if (!db)
        return id;

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT current_persona_id FROM character WHERE slot_id = ?",
                           -1, &stmt, nullptr) != SQLITE_OK)
        return id;
    sqlite3_bind_int(stmt, 1, slotId);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *text = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        if (text)
            id = text;
    }
    sqlite3_finalize(stmt);
    return id;
}

// ---- Legacy single-slot API (delegates to slot 1) ----

bool SaveRepository::saveAll(const Character &character, const Inventory &inventory,
                             const std::vector<std::shared_ptr<Persona>> &personas,
                             const SocialLinkManager &socialLinks, const QuestManager &quests)
{
    return saveAll(1, character, inventory, personas, socialLinks, quests);
}

bool SaveRepository::loadAll(Character &character, Inventory &inventory,
                             std::vector<std::shared_ptr<Persona>> &personas,
                             SocialLinkManager &socialLinks, QuestManager &quests)
{
    return loadAll(1, character, inventory, personas, socialLinks, quests);
}
