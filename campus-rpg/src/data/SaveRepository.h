#pragma once

class Character;
class Inventory;
class QuestManager;
class SocialLinkManager;
class Persona;

#include <memory>
#include <string>
#include <vector>

class SaveRepository
{
public:
    SaveRepository() = default;

    bool saveCharacter(const Character &character);
    bool loadCharacter(Character &character);

    bool saveInventory(const Inventory &inventory);
    bool loadInventory(Inventory &inventory);

    bool savePersonas(const std::vector<std::shared_ptr<Persona>> &personas);
    bool loadPersonas(std::vector<std::shared_ptr<Persona>> &personas, std::string &currentPersonaId);

    bool saveSocialLinks(const SocialLinkManager &manager);
    bool loadSocialLinks(SocialLinkManager &manager);

    bool saveQuests(const QuestManager &manager);
    bool loadQuests(QuestManager &manager);

    bool saveAll(const Character &character, const Inventory &inventory,
                 const std::vector<std::shared_ptr<Persona>> &personas,
                 const SocialLinkManager &socialLinks, const QuestManager &quests);
    bool loadAll(Character &character, Inventory &inventory,
                 std::vector<std::shared_ptr<Persona>> &personas,
                 SocialLinkManager &socialLinks, QuestManager &quests);
};
