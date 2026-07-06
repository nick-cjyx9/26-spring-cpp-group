#pragma once

#include <sqlite3.h>

#include <string>

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool initDatabase(const std::string &dbPath = "campus_rpg.db");
    void closeDatabase();
    bool isOpen() const;

    sqlite3 *database() const { return db_; }

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    std::string dbPath_;
    sqlite3 *db_ = nullptr;
    bool initialized_ = false;
};
