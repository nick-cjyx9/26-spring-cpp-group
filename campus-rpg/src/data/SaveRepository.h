#pragma once

class Character;
class Inventory;
class QuestManager;

class SaveRepository
{
public:
    SaveRepository() = default;

    bool saveCharacter(const Character &character);
    bool loadCharacter(Character &character);

    bool saveInventory(const Inventory &inventory);
    bool loadInventory(Inventory &inventory);

    bool saveQuests(const QuestManager &manager);
    bool loadQuests(QuestManager &manager);

    bool saveAll(const Character &character, const Inventory &inventory, const QuestManager &manager);
    bool loadAll(Character &character, Inventory &inventory, QuestManager &manager);
};
