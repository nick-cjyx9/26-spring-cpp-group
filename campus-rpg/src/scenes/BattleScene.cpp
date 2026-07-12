#include "BattleScene.h"
#include "GameManager.h"
#include "BattleSystem.h"
#include "Enemy.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"
#include "Element.h"
#include "Affinity.h"
#include "TileMap.h"
#include "Entity.h"
#include "Quest.h"
#include "QuestManager.h"

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    bool isBattleUsableItem(const Item *item)
    {
        return item && (item->type() == ItemType::Food || item->type() == ItemType::Potion ||
                        item->type() == ItemType::SpRecovery);
    }

    std::vector<size_t> battleItemIndices(const Inventory &inventory)
    {
        std::vector<size_t> result;
        for (size_t i = 0; i < inventory.size(); ++i)
        {
            if (isBattleUsableItem(inventory.itemAt(i)))
                result.push_back(i);
        }
        return result;
    }

    std::string costText(const Skill &skill)
    {
        if (skill.cost() <= 0)
            return "Free";
        return std::to_string(skill.cost()) + (skill.costType() == SkillCostType::HP ? " HP" : " SP");
    }

    std::string shortElementName(Element e)
    {
        switch (e)
        {
        case Element::Physical: return "Phys";
        case Element::Fire: return "Fire";
        case Element::Ice: return "Ice";
        case Element::Electric: return "Elec";
        case Element::Wind: return "Wind";
        case Element::Light: return "Light";
        case Element::Dark: return "Dark";
        case Element::Almighty: return "Almighty";
        default: return "?";
        }
    }

    void appendAffinity(std::string &out, const Enemy &enemy, Affinity wanted, const std::string &label)
    {
        std::string values;
        const Element elements[] = {Element::Physical, Element::Fire, Element::Ice, Element::Electric,
                                    Element::Wind, Element::Light, Element::Dark, Element::Almighty};
        for (Element e : elements)
        {
            if (enemy.affinity(e) == wanted)
            {
                if (!values.empty())
                    values += ",";
                values += shortElementName(e);
            }
        }
        if (!values.empty())
        {
            if (!out.empty())
                out += "  ";
            out += label + ": " + values;
        }
    }

    std::string affinitySummary(const Enemy &enemy)
    {
        std::string result;
        appendAffinity(result, enemy, Affinity::Weak, "Weak");
        appendAffinity(result, enemy, Affinity::Resist, "Resist");
        appendAffinity(result, enemy, Affinity::Null, "Null");
        appendAffinity(result, enemy, Affinity::Drain, "Drain");
        appendAffinity(result, enemy, Affinity::Repel, "Repel");
        return result.empty() ? "Affinity: Normal" : result;
    }

    void clampIndex(int &index, int count)
    {
        if (count <= 0)
        {
            index = 0;
            return;
        }
        if (index < 0)
            index = count - 1;
        if (index >= count)
            index = 0;
    }
} // namespace

void BattleScene::handleInput(engine::IInput &input)
{
    if (sleepTransition_)
        return;

    auto &battle = GameManager::instance().battleSystem();

    if (battle.isOver())
    {
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::Escape))
            returnAfterBattle();
        return;
    }

    if (!battle.isPlayerTurn())
        return;

    if (menu_ != BattleMenu::Main && input.wasKeyJustPressed(engine::Key::Escape))
    {
        menu_ = BattleMenu::Main;
        message_.clear();
        return;
    }

    switch (menu_)
    {
    case BattleMenu::Main:
        handleMainMenu(input);
        break;
    case BattleMenu::Skill:
        handleSkillMenu(input);
        break;
    case BattleMenu::Item:
        handleItemMenu(input);
        break;
    case BattleMenu::Persona:
        handlePersonaMenu(input);
        break;
    }
}

void BattleScene::handleMainMenu(engine::IInput &input)
{
    auto &battle = GameManager::instance().battleSystem();
    const int actionCount = 5;
    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedAction_ = (selectedAction_ + actionCount - 1) % actionCount;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedAction_ = (selectedAction_ + 1) % actionCount;

    if (!(input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)))
        return;

    auto &character = GameManager::instance().character();
    switch (selectedAction_)
    {
    case 0:
        battle.playerAttack(0);
        message_.clear();
        break;
    case 1:
        menu_ = BattleMenu::Skill;
        selectedSkill_ = 0;
        message_.clear();
        break;
    case 2:
        menu_ = BattleMenu::Item;
        selectedItem_ = 0;
        message_.clear();
        break;
    case 3:
        menu_ = BattleMenu::Persona;
        selectedPersona_ = 0;
        if (Persona *p = character.currentPersona())
        {
            const auto &owned = character.ownedPersonas();
            for (size_t i = 0; i < owned.size(); ++i)
            {
                if (owned[i] && owned[i]->id() == p->id())
                {
                    selectedPersona_ = static_cast<int>(i);
                    break;
                }
            }
        }
        message_.clear();
        break;
    case 4:
        battle.playerFlee();
        message_.clear();
        break;
    }
}

void BattleScene::handleSkillMenu(engine::IInput &input)
{
    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    Persona *persona = character.currentPersona();
    int count = persona ? static_cast<int>(persona->skills().size()) : 0;
    if (count <= 0)
    {
        message_ = "No skills available.";
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
            menu_ = BattleMenu::Main;
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up))
        --selectedSkill_;
    if (input.wasKeyJustPressed(engine::Key::Down))
        ++selectedSkill_;
    clampIndex(selectedSkill_, count);

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        const auto &skill = persona->skills()[static_cast<size_t>(selectedSkill_)];
        if (!skill)
            return;
        if (!skill->canUse(character))
        {
            message_ = "Not enough " + std::string(skill->costType() == SkillCostType::HP ? "HP." : "SP.");
            return;
        }
        if (battle.playerUseSkill(0, *skill))
        {
            menu_ = BattleMenu::Main;
            message_.clear();
        }
    }
}

void BattleScene::handleItemMenu(engine::IInput &input)
{
    auto &battle = GameManager::instance().battleSystem();
    Inventory &inventory = GameManager::instance().inventory();
    auto usable = battleItemIndices(inventory);
    int count = static_cast<int>(usable.size());
    if (count <= 0)
    {
        message_ = "No usable battle items.";
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
            menu_ = BattleMenu::Main;
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up))
        --selectedItem_;
    if (input.wasKeyJustPressed(engine::Key::Down))
        ++selectedItem_;
    clampIndex(selectedItem_, count);

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        size_t inventoryIndex = usable[static_cast<size_t>(selectedItem_)];
        Item *item = inventory.itemAt(inventoryIndex);
        if (item && battle.playerUseItem(item->clone()))
        {
            std::string itemName = item->name();
            inventory.removeItem(inventoryIndex);
            battle.appendLog("Consumed " + itemName + ".");
            menu_ = BattleMenu::Main;
            message_.clear();
        }
    }
}

void BattleScene::handlePersonaMenu(engine::IInput &input)
{
    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    const auto &owned = character.ownedPersonas();
    int count = static_cast<int>(owned.size());
    if (count <= 0)
    {
        message_ = "No Personas owned.";
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
            menu_ = BattleMenu::Main;
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up))
        --selectedPersona_;
    if (input.wasKeyJustPressed(engine::Key::Down))
        ++selectedPersona_;
    clampIndex(selectedPersona_, count);

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        auto selected = owned[static_cast<size_t>(selectedPersona_)];
        Persona *current = character.currentPersona();
        if (!selected)
            return;
        if (current && selected->id() == current->id())
        {
            message_ = "Already equipped.";
            return;
        }
        if (battle.playerSwitchPersona(selected))
        {
            menu_ = BattleMenu::Main;
            message_.clear();
        }
    }
}

void BattleScene::update(float deltaTime)
{
    if (!sleepTransition_)
        return;

    sleepTimer_ += deltaTime;
    if (sleepTimer_ >= 2.2f)
        finishSleepTransition();
}

void BattleScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    bool backgroundSecondMap = sleepTransition_ ? sleepStartedOnSecondMap_ : GameManager::instance().onSecondMap();
    renderer.drawTexture(backgroundSecondMap ? "town_bg2" : "town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 20, 200));

    if (sleepTransition_)
    {
        float alpha = 255.0f;
        if (sleepTimer_ < 0.45f)
            alpha = sleepTimer_ / 0.45f * 255.0f;
        else if (sleepTimer_ > 1.35f)
            alpha = std::max(0.0f, (2.2f - sleepTimer_) / 0.85f * 255.0f);

        renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, static_cast<unsigned char>(alpha)));
        if (sleepTimer_ >= 0.45f && sleepTimer_ <= 1.55f)
            renderer.drawText("Returning home... Resting until morning...", {210, 285}, 20, engine::Color::white());
        return;
    }

    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    Enemy *enemy = battle.enemyAt(0);
    Persona *persona = character.currentPersona();

    // Enemy panel
    if (enemy)
    {
        renderer.drawTexture("shadow", {520, 95, 96, 96});
        renderer.drawText(enemy->name(), {500, 60}, 22, engine::Color::white());
        renderer.drawText("HP " + std::to_string(enemy->hp()) + "/" + std::to_string(enemy->maxHp()),
                          {500, 205}, 18, engine::Color::red());
        renderer.drawText("STR " + std::to_string(enemy->strength()) + "  MAG " + std::to_string(enemy->magic()) +
                              "  SPD " + std::to_string(enemy->speed()),
                          {500, 230}, 14, engine::Color::white());
        renderer.drawText(affinitySummary(*enemy), {500, 252}, 12, engine::Color::cyan());
    }

    // Player panel
    renderer.drawTexture("player", {145, 250, 64, 64});
    renderer.drawText(character.name(), {145, 225}, 22, engine::Color::white());
    renderer.drawText("HP " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()) +
                          "  SP " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()),
                      {145, 330}, 16, engine::Color::green());
    if (persona)
    {
        renderer.drawText("Persona: " + persona->name() + " Lv" + std::to_string(persona->level()),
                          {145, 353}, 15, engine::Color::cyan());
        renderer.drawText("STR " + std::to_string(character.attack()) + "  MAG " + std::to_string(character.magic()) +
                              "  SPD " + std::to_string(character.speed()),
                          {145, 374}, 14, engine::Color::white());
    }

    // Menu panel
    renderer.drawRect({20, 420, 260, 160}, engine::Color(0, 0, 0, 210));
    if (menu_ == BattleMenu::Main)
    {
        const char *actions[] = {"Attack", "Skill", "Item", "Switch Persona", "Flee"};
        for (int i = 0; i < 5; ++i)
        {
            engine::Color color = (i == selectedAction_) ? engine::Color::yellow() : engine::Color::white();
            renderer.drawText(std::string((i == selectedAction_ ? "> " : "  ")) + actions[i],
                              {35.0f, 435.0f + i * 24.0f}, 18, color);
        }
    }
    else if (menu_ == BattleMenu::Skill)
    {
        renderer.drawText("Skills (Esc: back)", {35, 432}, 16, engine::Color::cyan());
        if (!persona || persona->skills().empty())
        {
            renderer.drawText("No skills available.", {35, 460}, 15, engine::Color::gray());
        }
        else
        {
            const auto &skills = persona->skills();
            for (size_t i = 0; i < skills.size() && i < 5; ++i)
            {
                if (!skills[i])
                    continue;
                bool selected = static_cast<int>(i) == selectedSkill_;
                bool canUse = skills[i]->canUse(character);
                engine::Color color = selected ? engine::Color::yellow() : (canUse ? engine::Color::white() : engine::Color::gray());
                renderer.drawText(std::string(selected ? "> " : "  ") + skills[i]->name(),
                                  {35.0f, 458.0f + static_cast<float>(i) * 22.0f}, 15, color);
                renderer.drawText(costText(*skills[i]) + " " + shortElementName(skills[i]->element()) + " P" + std::to_string(skills[i]->power()),
                                  {150.0f, 458.0f + static_cast<float>(i) * 22.0f}, 13, color);
            }
        }
    }
    else if (menu_ == BattleMenu::Item)
    {
        renderer.drawText("Items (Esc: back)", {35, 432}, 16, engine::Color::cyan());
        auto usable = battleItemIndices(GameManager::instance().inventory());
        if (usable.empty())
        {
            renderer.drawText("No usable battle items.", {35, 460}, 15, engine::Color::gray());
        }
        else
        {
            for (size_t row = 0; row < usable.size() && row < 5; ++row)
            {
                Item *item = GameManager::instance().inventory().itemAt(usable[row]);
                if (!item)
                    continue;
                bool selected = static_cast<int>(row) == selectedItem_;
                engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
                renderer.drawText(std::string(selected ? "> " : "  ") + item->name(),
                                  {35.0f, 458.0f + static_cast<float>(row) * 22.0f}, 15, color);
                renderer.drawText(item->typeString(), {180.0f, 458.0f + static_cast<float>(row) * 22.0f}, 13, engine::Color::gray());
            }
        }
    }
    else if (menu_ == BattleMenu::Persona)
    {
        renderer.drawText("Persona (Esc: back)", {35, 432}, 16, engine::Color::cyan());
        const auto &owned = character.ownedPersonas();
        if (owned.empty())
        {
            renderer.drawText("No Personas owned.", {35, 460}, 15, engine::Color::gray());
        }
        else
        {
            for (size_t i = 0; i < owned.size() && i < 5; ++i)
            {
                if (!owned[i])
                    continue;
                bool selected = static_cast<int>(i) == selectedPersona_;
                bool current = persona && owned[i]->id() == persona->id();
                engine::Color color = selected ? engine::Color::yellow() : engine::Color::white();
                std::string line = std::string(selected ? "> " : "  ") + owned[i]->name() + " Lv" + std::to_string(owned[i]->level());
                if (current)
                    line += " *";
                renderer.drawText(line, {35.0f, 458.0f + static_cast<float>(i) * 22.0f}, 15, color);
                renderer.drawText(std::to_string(owned[i]->strength()) + "/" + std::to_string(owned[i]->magic()) + "/" + std::to_string(owned[i]->speed()),
                                  {185.0f, 458.0f + static_cast<float>(i) * 22.0f}, 13, engine::Color::gray());
            }
        }
    }

    // Battle log panel
    renderer.drawRect({300, 420, 470, 160}, engine::Color(0, 0, 0, 210));
    renderer.drawText("Log:", {315, 435}, 18, engine::Color::white());
    const auto &log = battle.log();
    size_t start = log.size() > 6 ? log.size() - 6 : 0;
    for (size_t i = start; i < log.size(); ++i)
    {
        renderer.drawText(log[i], {315, 460.0f + static_cast<float>(i - start) * 19.0f}, 14, engine::Color::white());
    }
    if (!message_.empty())
        renderer.drawText(message_, {315, 558}, 14, engine::Color::red());

    if (battle.isOver())
    {
        if (!postBattleHandled_)
            returnAfterBattle();
        return;
    }
}

bool BattleScene::processVictory()
{
    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    int totalExp = 0;
    int totalGold = 0;

    for (const auto &enemyPtr : battle.enemies())
    {
        if (!enemyPtr)
            continue;
        totalExp += enemyPtr->rewardExp();
        totalGold += enemyPtr->rewardGold();
        for (const auto &pid : enemyPtr->dropPersonaIds())
        {
            auto dropped = GameManager::instance().findPersona(pid);
            if (dropped)
            {
                GameManager::instance().addPersonaToPlayer(dropped);
                battle.appendLog("Obtained Persona: " + dropped->name());
            }
        }
    }

    character.gainExp(totalExp);
    character.addGold(totalGold);

    // Update kill-quest progress for accepted kill quests.
    {
        auto &qm = GameManager::instance().questManager();
        int enemyCount = 0;
        for (const auto &enemyPtr : battle.enemies())
        {
            if (enemyPtr)
                ++enemyCount;
        }
        for (auto *q : qm.acceptedQuests())
        {
            if (!q || q->type() != QuestType::Kill)
                continue;
            q->setCurrentProgress(q->currentProgress() + enemyCount);
            if (q->currentProgress() >= q->targetCount())
                q->complete();
        }
    }

    return character.hasLevelUpSnapshot();
}

void BattleScene::processDefeat()
{
    auto &character = GameManager::instance().character();
    character.heal(character.maxHp());
    character.recoverSp(character.maxSp());
}

void BattleScene::returnAfterBattle()
{
    auto &gm = GameManager::instance();
    auto &battle = gm.battleSystem();

    sleepLeveledUp_ = false;
    if (battle.playerWon())
        sleepLeveledUp_ = processVictory();

    postBattleHandled_ = true;
    beginSleepTransition();
}

void BattleScene::beginSleepTransition()
{
    auto &gm = GameManager::instance();
    auto &character = gm.character();

    sleepStartedOnSecondMap_ = gm.onSecondMap();

    character.heal(character.maxHp());
    character.recoverSp(character.maxSp());

    gm.setNight(false);
    gm.setOnSecondMap(false);
    gm.advanceDay();

    TileMap &town = gm.townMap();
    const engine::Vec2 homePos{610.0f, 105.0f};
    bool movedPlayer = false;
    for (const auto &e : town.entities())
    {
        if (e && e->type() == "player")
        {
            static_cast<PlayerEntity *>(e.get())->setPosition(homePos);
            movedPlayer = true;
            break;
        }
    }
    if (!movedPlayer)
        town.addEntity(std::make_unique<PlayerEntity>(homePos));

    sleepTransition_ = true;
    sleepTimer_ = 0.0f;
}

void BattleScene::finishSleepTransition()
{
    sleepTransition_ = false;
    auto &gm = GameManager::instance();
    if (sleepLeveledUp_)
        gm.enterScene(SceneType::LevelUp);
    else
        gm.enterScene(SceneType::Town);
}
