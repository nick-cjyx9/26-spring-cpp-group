#include "DatabaseManager.h"

#include <iostream>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initDatabase(const std::string &dbPath)
{
    dbPath_ = dbPath;

    if (sqlite3_open(dbPath_.c_str(), &db_) != SQLITE_OK)
    {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    // Schema versioning. Version 1 = multi-slot save schema (slot_id columns).
    int version = 0;
    sqlite3_stmt *vstmt = nullptr;
    if (sqlite3_prepare_v2(db_, "PRAGMA user_version", -1, &vstmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(vstmt) == SQLITE_ROW)
            version = sqlite3_column_int(vstmt, 0);
        sqlite3_finalize(vstmt);
    }

    // Helper: does `table` have a column named `col`?
    auto hasColumn = [this](const char *table, const char *col) -> bool {
        std::string pragma = std::string("PRAGMA table_info(") + table + ")";
        sqlite3_stmt *info = nullptr;
        if (sqlite3_prepare_v2(db_, pragma.c_str(), -1, &info, nullptr) != SQLITE_OK)
            return false;
        bool found = false;
        while (sqlite3_step(info) == SQLITE_ROW)
        {
            const char *cn = reinterpret_cast<const char *>(sqlite3_column_text(info, 1));
            if (cn && std::string(cn) == col)
            {
                found = true;
                break;
            }
        }
        sqlite3_finalize(info);
        return found;
    };

    bool hasLegacyCharacter = false;
    // A v1+ DB whose `character` table still uses the old 5-stat combat
    // columns (eq_atk/eq_def, no eq_str) is incompatible with the current
    // 3-stat mechanism. Treat it as a schema mismatch and rebuild the tables
    // (old saves under the obsolete schema cannot be restored anyway).
    bool staleV1Schema = (version >= 1) && hasColumn("character", "slot_id") &&
                         !hasColumn("character", "eq_str");
    if (staleV1Schema)
        version = 0; // fall through to the fresh-DB recreate path below
    if (version < 1)
    {
        // Detect legacy single-slot schema: a `character` table that has an
        // `id` column but no `slot_id` column. We migrate (not drop) its data
        // into slot_id=1 of the new schema so existing saves are preserved.
        hasLegacyCharacter = false;
        sqlite3_stmt *infoStmt = nullptr;
        if (sqlite3_prepare_v2(db_, "PRAGMA table_info(character)", -1, &infoStmt, nullptr) == SQLITE_OK)
        {
            bool hasId = false;
            bool hasSlotId = false;
            while (sqlite3_step(infoStmt) == SQLITE_ROW)
            {
                const char *col = reinterpret_cast<const char *>(sqlite3_column_text(infoStmt, 1));
                if (col)
                {
                    std::string n = col;
                    if (n == "id")
                        hasId = true;
                    else if (n == "slot_id")
                        hasSlotId = true;
                }
            }
            sqlite3_finalize(infoStmt);
            hasLegacyCharacter = hasId && !hasSlotId;
        }

        if (hasLegacyCharacter)
        {
            // Rename legacy tables out of the way (data preserved), then create
            // the new schema below, then copy legacy rows into slot_id=1.
            sqlite3_exec(db_, "ALTER TABLE character RENAME TO character_legacy;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "ALTER TABLE persona RENAME TO persona_legacy;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "ALTER TABLE inventory RENAME TO inventory_legacy;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "ALTER TABLE social_link RENAME TO social_link_legacy;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "ALTER TABLE quest_progress RENAME TO quest_progress_legacy;", nullptr, nullptr, nullptr);
        }
        else
        {
            // No legacy schema (fresh DB, or stale v1 being rebuilt). Make sure
            // no stale partially-created new tables linger before recreating.
            sqlite3_exec(db_, "DROP TABLE IF EXISTS character;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS persona;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS inventory;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS social_link;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS quest_progress;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS game_state;", nullptr, nullptr, nullptr);
            sqlite3_exec(db_, "DROP TABLE IF EXISTS save_meta;", nullptr, nullptr, nullptr);
        }
    }

    const char *createSql = R"(
        CREATE TABLE IF NOT EXISTS character (
            slot_id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            level INTEGER DEFAULT 1,
            hp INTEGER DEFAULT 100,
            max_hp INTEGER DEFAULT 100,
            sp INTEGER DEFAULT 50,
            max_sp INTEGER DEFAULT 50,
            exp INTEGER DEFAULT 0,
            exp_to_next INTEGER DEFAULT 100,
            gold INTEGER DEFAULT 0,
            eq_str INTEGER DEFAULT 0,
            eq_mag INTEGER DEFAULT 0,
            eq_spd INTEGER DEFAULT 0,
            pos_x REAL DEFAULT 0,
            pos_y REAL DEFAULT 0,
            is_night INTEGER DEFAULT 0,
            current_persona_id TEXT
        );

        CREATE TABLE IF NOT EXISTS persona (
            slot_id INTEGER NOT NULL DEFAULT 1,
            id TEXT NOT NULL,
            name TEXT NOT NULL,
            arcana TEXT NOT NULL,
            level INTEGER DEFAULT 1,
            strength INTEGER DEFAULT 5,
            magic INTEGER DEFAULT 5,
            speed INTEGER DEFAULT 5,
            affinities TEXT,
            skills TEXT,
            owner TEXT,
            PRIMARY KEY (slot_id, id)
        );

        CREATE TABLE IF NOT EXISTS inventory (
            slot_id INTEGER NOT NULL DEFAULT 1,
            ord INTEGER,
            item_id TEXT NOT NULL,
            item_type TEXT NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            value INTEGER DEFAULT 0,
            extra_data TEXT
        );

        CREATE TABLE IF NOT EXISTS social_link (
            slot_id INTEGER NOT NULL DEFAULT 1,
            id TEXT NOT NULL,
            rank INTEGER DEFAULT 0,
            points INTEGER DEFAULT 0,
            name TEXT,
            portrait TEXT,
            PRIMARY KEY (slot_id, id)
        );

        CREATE TABLE IF NOT EXISTS quest_progress (
            slot_id INTEGER NOT NULL DEFAULT 1,
            quest_id TEXT NOT NULL,
            accepted INTEGER DEFAULT 0,
            completed INTEGER DEFAULT 0,
            rewarded INTEGER DEFAULT 0,
            PRIMARY KEY (slot_id, quest_id)
        );

        CREATE TABLE IF NOT EXISTS game_state (
            slot_id INTEGER PRIMARY KEY,
            day INTEGER DEFAULT 1
        );

        CREATE TABLE IF NOT EXISTS save_meta (
            slot_id INTEGER PRIMARY KEY,
            character_name TEXT,
            level INTEGER DEFAULT 1,
            updated_at TEXT
        );
    )";

    char *errMsg = nullptr;
    if (sqlite3_exec(db_, createSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Failed to create tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }

    // Schema v2: ensure social_link has name/portrait columns (persistent NPC
    // identity). Fresh DBs already get them from CREATE TABLE above; existing
    // v1 DBs need an in-place ALTER. Idempotent via PRAGMA table_info check.
    auto addColumnIfMissing = [this](const char *table, const char *col, const char *typeDef) {
        sqlite3_stmt *info = nullptr;
        std::string pragma = std::string("PRAGMA table_info(") + table + ")";
        if (sqlite3_prepare_v2(db_, pragma.c_str(), -1, &info, nullptr) != SQLITE_OK)
            return;
        bool found = false;
        while (sqlite3_step(info) == SQLITE_ROW)
        {
            const char *cn = reinterpret_cast<const char *>(sqlite3_column_text(info, 1));
            if (cn && std::string(cn) == col)
            {
                found = true;
                break;
            }
        }
        sqlite3_finalize(info);
        if (!found)
        {
            std::string alter = std::string("ALTER TABLE ") + table + " ADD COLUMN " + col + " " + typeDef + ";";
            sqlite3_exec(db_, alter.c_str(), nullptr, nullptr, nullptr);
        }
    };
    addColumnIfMissing("social_link", "name", "TEXT");
    addColumnIfMissing("social_link", "portrait", "TEXT");

    sqlite3_exec(db_, "CREATE TABLE IF NOT EXISTS game_state (slot_id INTEGER PRIMARY KEY, day INTEGER DEFAULT 1);",
                 nullptr, nullptr, nullptr);

    if (version < 1)
    {
        // If legacy tables were renamed, copy their data into slot_id=1 of the
        // new schema (non-destructive migration), then drop the legacy tables.
        // Only run when the legacy schema was actually present.
        if (hasLegacyCharacter)
        {
            const char *migrateSql = R"(
                INSERT INTO character (slot_id, name, level, hp, max_hp, sp, max_sp,
                                       exp, exp_to_next, gold, pos_x, pos_y, is_night,
                                       current_persona_id)
                SELECT 1, name, level, hp, max_hp, sp, max_sp, exp, exp_to_next, gold,
                       pos_x, pos_y, is_night, current_persona_id
                FROM character_legacy;

                INSERT INTO persona (slot_id, id, name, arcana, level, st, ma, en, ag, lu,
                                     affinities, skills, owner)
                SELECT 1, id, name, arcana, level, st, ma, en, ag, lu, affinities, skills, owner
                FROM persona_legacy;

                INSERT INTO inventory (slot_id, ord, item_id, item_type, name, description,
                                       value, extra_data)
                SELECT 1, slot, item_id, item_type, name, description, value, extra_data
                FROM inventory_legacy;

                INSERT INTO social_link (slot_id, id, rank, points)
                SELECT 1, id, rank, points
                FROM social_link_legacy;

                INSERT INTO quest_progress (slot_id, quest_id, accepted, completed, rewarded)
                SELECT 1, quest_id, accepted, completed, rewarded
                FROM quest_progress_legacy;

                INSERT INTO game_state (slot_id, day)
                VALUES (1, 1);

                INSERT INTO save_meta (slot_id, character_name, level, updated_at)
                SELECT 1, name, level, 'migrated'
                FROM character WHERE slot_id = 1;

                DROP TABLE IF EXISTS character_legacy;
                DROP TABLE IF EXISTS persona_legacy;
                DROP TABLE IF EXISTS inventory_legacy;
                DROP TABLE IF EXISTS social_link_legacy;
                DROP TABLE IF EXISTS quest_progress_legacy;
            )";
            char *migrateErr = nullptr;
            if (sqlite3_exec(db_, migrateSql, nullptr, nullptr, &migrateErr) != SQLITE_OK)
            {
                std::cerr << "Save migration warning: " << (migrateErr ? migrateErr : "unknown") << "\n";
                sqlite3_free(migrateErr);
            }
        }

        sqlite3_exec(db_, "PRAGMA user_version = 2;", nullptr, nullptr, nullptr);
    }

    initialized_ = true;
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (db_)
    {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    initialized_ = false;
}

bool DatabaseManager::isOpen() const
{
    return initialized_ && db_ != nullptr;
}
