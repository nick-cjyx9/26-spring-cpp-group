#pragma once

#include <QString>
#include <QSqlDatabase>

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool initDatabase(const QString &dbPath = QStringLiteral("campus_rpg.db"));
    void closeDatabase();
    bool isOpen() const;

    QSqlDatabase database() const;

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    QString dbPath_;
    bool initialized_ = false;
};
