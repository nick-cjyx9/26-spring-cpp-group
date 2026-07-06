#include "DatabaseManager.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::~DatabaseManager()
{
    closeDatabase();
}

bool DatabaseManager::initDatabase(const QString &dbPath)
{
    dbPath_ = dbPath;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath_);

    if (!db.open())
    {
        qWarning() << "Failed to open database:" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    bool ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS character (
            id INTEGER PRIMARY KEY CHECK(id = 1),
            name TEXT NOT NULL,
            level INTEGER DEFAULT 1,
            hp INTEGER DEFAULT 100,
            max_hp INTEGER DEFAULT 100,
            exp INTEGER DEFAULT 0,
            exp_to_next_level INTEGER DEFAULT 100,
            gold INTEGER DEFAULT 0,
            base_attack INTEGER DEFAULT 10,
            base_defense INTEGER DEFAULT 5,
            equipment_attack_bonus INTEGER DEFAULT 0,
            equipment_defense_bonus INTEGER DEFAULT 0
        )
    )");
    if (!ok)
    {
        qWarning() << "Failed to create character table:" << query.lastError().text();
        return false;
    }

    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS inventory (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            item_id TEXT NOT NULL,
            item_type TEXT NOT NULL,
            name TEXT NOT NULL,
            description TEXT,
            value INTEGER DEFAULT 0,
            extra_data TEXT
        )
    )");
    if (!ok)
    {
        qWarning() << "Failed to create inventory table:" << query.lastError().text();
        return false;
    }

    ok = query.exec(R"(
        CREATE TABLE IF NOT EXISTS quest_progress (
            quest_id TEXT PRIMARY KEY,
            accepted INTEGER DEFAULT 0,
            completed INTEGER DEFAULT 0,
            rewarded INTEGER DEFAULT 0
        )
    )");
    if (!ok)
    {
        qWarning() << "Failed to create quest_progress table:" << query.lastError().text();
        return false;
    }

    initialized_ = true;
    return true;
}

void DatabaseManager::closeDatabase()
{
    if (!initialized_)
        return;
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    initialized_ = false;
}

bool DatabaseManager::isOpen() const
{
    return initialized_ && QSqlDatabase::database().isOpen();
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database();
}
