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

    if (version < 1)
    {
        // Drop legacy single-slot tables (old schema had `id CHECK(id=1)` on
        // `character`). Saves from the old schema are incompatible with the
        // multi-slot layout, so we drop and recreate. Fresh DBs have nothing
        // to drop.
        sqlite3_exec(db_, "DROP TABLE IF EXISTS character;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "DROP TABLE IF EXISTS persona;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "DROP TABLE IF EXISTS inventory;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "DROP TABLE IF EXISTS social_link;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "DROP TABLE IF EXISTS quest_progress;", nullptr, nullptr, nullptr);
        sqlite3_exec(db_, "DROP TABLE IF EXISTS save_meta;", nullptr, nullptr, nullptr);
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
            st INTEGER DEFAULT 5,
            ma INTEGER DEFAULT 5,
            en INTEGER DEFAULT 5,
            ag INTEGER DEFAULT 5,
            lu INTEGER DEFAULT 5,
            eq_atk INTEGER DEFAULT 0,
            eq_def INTEGER DEFAULT 0,
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
            st INTEGER DEFAULT 5,
            ma INTEGER DEFAULT 5,
            en INTEGER DEFAULT 5,
            ag INTEGER DEFAULT 5,
            lu INTEGER DEFAULT 5,
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

    if (version < 1)
        sqlite3_exec(db_, "PRAGMA user_version = 1;", nullptr, nullptr, nullptr);

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
