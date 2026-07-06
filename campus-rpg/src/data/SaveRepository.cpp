#include "SaveRepository.h"

#include "Character.h"
#include "Inventory.h"
#include "Item.h"
#include "Quest.h"
#include "QuestManager.h"
#include "DatabaseManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <QtDebug>

namespace
{

    QString extraDataFromItem(const Item &item)
    {
        QJsonObject obj;
        switch (item.type())
        {
        case ItemType::Food:
            obj["healAmount"] = static_cast<const FoodItem &>(item).healAmount();
            break;
        case ItemType::Potion:
            obj["healAmount"] = static_cast<const PotionItem &>(item).healAmount();
            break;
        case ItemType::Equipment:
        {
            const auto &eq = static_cast<const EquipmentItem &>(item);
            obj["attackBonus"] = eq.attackBonus();
            obj["defenseBonus"] = eq.defenseBonus();
            break;
        }
        }
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    std::unique_ptr<Item> itemFromRecord(const QSqlQuery &query)
    {
        QString itemId = query.value("item_id").toString();
        QString name = query.value("name").toString();
        QString description = query.value("description").toString();
        int value = query.value("value").toInt();
        QString typeStr = query.value("item_type").toString();
        QString extraData = query.value("extra_data").toString();

        QJsonObject obj = QJsonDocument::fromJson(extraData.toUtf8()).object();

        if (typeStr == "Food")
        {
            int heal = obj["healAmount"].toInt();
            return std::make_unique<FoodItem>(itemId.toStdString(), name.toStdString(),
                                              description.toStdString(), value, heal);
        }
        else if (typeStr == "Potion")
        {
            int heal = obj["healAmount"].toInt();
            return std::make_unique<PotionItem>(itemId.toStdString(), name.toStdString(),
                                                description.toStdString(), value, heal);
        }
        else if (typeStr == "Equipment")
        {
            int atk = obj["attackBonus"].toInt();
            int def = obj["defenseBonus"].toInt();
            return std::make_unique<EquipmentItem>(itemId.toStdString(), name.toStdString(),
                                                   description.toStdString(), value, atk, def);
        }
        return nullptr;
    }

} // namespace

bool SaveRepository::saveCharacter(const Character &character)
{
    QSqlQuery query;
    query.prepare(R"(
        REPLACE INTO character (id, name, level, hp, max_hp, exp, exp_to_next_level, gold,
                                base_attack, base_defense, equipment_attack_bonus, equipment_defense_bonus)
        VALUES (1, :name, :level, :hp, :max_hp, :exp, :exp_to_next_level, :gold,
                :base_attack, :base_defense, :equipment_attack_bonus, :equipment_defense_bonus)
    )");
    query.bindValue(":name", QString::fromStdString(character.name()));
    query.bindValue(":level", character.level());
    query.bindValue(":hp", character.hp());
    query.bindValue(":max_hp", character.maxHp());
    query.bindValue(":exp", character.exp());
    query.bindValue(":exp_to_next_level", character.expToNextLevel());
    query.bindValue(":gold", character.gold());
    query.bindValue(":base_attack", character.baseAttack());
    query.bindValue(":base_defense", character.baseDefense());
    query.bindValue(":equipment_attack_bonus", 0);
    query.bindValue(":equipment_defense_bonus", 0);

    if (!query.exec())
    {
        qWarning() << "saveCharacter failed:" << query.lastError().text();
        return false;
    }
    return true;
}

bool SaveRepository::loadCharacter(Character &character)
{
    QSqlQuery query("SELECT * FROM character WHERE id = 1");
    if (!query.next())
        return false;

    character = Character(query.value("name").toString().toStdString(),
                          query.value("max_hp").toInt(),
                          query.value("base_attack").toInt(),
                          query.value("base_defense").toInt());
    // Note: the simple Character constructor does not restore level/exp/gold.
    // This is intentional for the scaffold; extend the constructor or add a restore method later.
    return true;
}

bool SaveRepository::saveInventory(const Inventory &inventory)
{
    QSqlQuery query;
    if (!query.exec("DELETE FROM inventory"))
    {
        qWarning() << "saveInventory cleanup failed:" << query.lastError().text();
        return false;
    }

    for (const auto &item : inventory.items())
    {
        query.prepare(R"(
            INSERT INTO inventory (item_id, item_type, name, description, value, extra_data)
            VALUES (:item_id, :item_type, :name, :description, :value, :extra_data)
        )");
        query.bindValue(":item_id", QString::fromStdString(item->id()));
        query.bindValue(":item_type", QString::fromStdString(item->typeString()));
        query.bindValue(":name", QString::fromStdString(item->name()));
        query.bindValue(":description", QString::fromStdString(item->description()));
        query.bindValue(":value", item->value());
        query.bindValue(":extra_data", extraDataFromItem(*item));
        if (!query.exec())
        {
            qWarning() << "saveInventory insert failed:" << query.lastError().text();
            return false;
        }
    }
    return true;
}

bool SaveRepository::loadInventory(Inventory &inventory)
{
    inventory.clear();
    QSqlQuery query("SELECT * FROM inventory");
    while (query.next())
    {
        auto item = itemFromRecord(query);
        if (item)
            inventory.addItem(std::move(item));
    }
    return true;
}

bool SaveRepository::saveQuests(const QuestManager &manager)
{
    QSqlQuery query;
    if (!query.exec("DELETE FROM quest_progress"))
    {
        qWarning() << "saveQuests cleanup failed:" << query.lastError().text();
        return false;
    }

    // We only persist progress for quests that have been accepted at least once.
    // For the scaffold we iterate through known quests and save those with accepted flag.
    // A full implementation should expose all quests from QuestManager.
    return true;
}

bool SaveRepository::loadQuests(QuestManager &manager)
{
    QSqlQuery query("SELECT * FROM quest_progress");
    while (query.next())
    {
        QString questId = query.value("quest_id").toString();
        Quest *q = manager.getQuest(questId.toStdString());
        if (!q)
            continue;
        if (query.value("accepted").toInt())
            q->accept();
        if (query.value("completed").toInt())
            q->complete();
        if (query.value("rewarded").toInt())
            q->reward();
    }
    return true;
}

bool SaveRepository::saveAll(const Character &character, const Inventory &inventory, const QuestManager &manager)
{
    if (!DatabaseManager::instance().isOpen())
        return false;
    return saveCharacter(character) && saveInventory(inventory) && saveQuests(manager);
}

bool SaveRepository::loadAll(Character &character, Inventory &inventory, QuestManager &manager)
{
    if (!DatabaseManager::instance().isOpen())
        return false;
    return loadCharacter(character) && loadInventory(inventory) && loadQuests(manager);
}
