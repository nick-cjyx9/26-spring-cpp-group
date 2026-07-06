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

    const char *createSql = R"(
        CREATE TABLE IF NOT EXISTS character (
            id INTEGER PRIMARY KEY CHECK(id = 1),
            name TEXT NOT NULL,
            level INTEGER DEFAULT 1,
            hp INTEGER DEFAULT 100,
            max_hp INTEGER DEFAULT 100,
            sp INTEGER DEFAULT 50,
            max_sp INTEGER DEFAULT 50,
            exp INTEGER DEFAULT 0,
            exp_to_next INTEGER DEFAULT 100,
            gold INTEGER DEFAULT 0,
            pos_x REAL DEFAULT 0,
            pos_y REAL DEFAULT 0,
            is_night INTEGER DEFAULT 0,
            current_persona_id TEXT
        );

        CREATE TABLE IF NOT EXISTS persona (
            id TEXT PRIMARY KEY,
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
            owner TEXT
        );

        CREATE TABLE IF NOT EXISTS inventory (
            slot INTEGER PRIMARY KEY AUTOINCREMENT,
            item_id TEXT NOT NULL,
            item_type TEXT NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            value INTEGER DEFAULT 0,
            extra_data TEXT
        );

        CREATE TABLE IF NOT EXISTS social_link (
            id TEXT PRIMARY KEY,
            rank INTEGER DEFAULT 0,
            points INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS quest_progress (
            quest_id TEXT PRIMARY KEY,
            accepted INTEGER DEFAULT 0,
            completed INTEGER DEFAULT 0,
            rewarded INTEGER DEFAULT 0
        );
    )";

    char *errMsg = nullptr;
    if (sqlite3_exec(db_, createSql, nullptr, nullptr, &errMsg) != SQLITE_OK)
    {
        std::cerr << "Failed to create tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
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
