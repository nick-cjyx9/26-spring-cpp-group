#include "BattleScene.h"
#include "GameManager.h"
#include "BattleSystem.h"
#include "Enemy.h"
#include "Inventory.h"
#include "Item.h"
#include "Persona.h"

void BattleScene::handleInput(engine::IInput &input)
{
    auto &battle = GameManager::instance().battleSystem();

    if (battle.isOver())
    {
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::Escape))
        {
            if (battle.playerWon())
            {
                bool leveledUp = processVictory();
                battle.recoverPlayerSp();
                if (leveledUp)
                {
                    GameManager::instance().enterScene(SceneType::LevelUp);
                    return;
                }
            }
            else
            {
                processDefeat();
            }
            GameManager::instance().setNight(false);
            // Returning from night (battle) to day = next day: refresh NPCs.
            GameManager::instance().advanceDay();
            GameManager::instance().enterScene(SceneType::Town);
        }
        return;
    }

    // Only accept player commands during the player's turn.
    if (!battle.isPlayerTurn())
        return;

    const int actionCount = 5;
    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedAction_ = (selectedAction_ + actionCount - 1) % actionCount;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedAction_ = (selectedAction_ + 1) % actionCount;

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        auto &character = GameManager::instance().character();
        Persona *p = character.currentPersona();

        switch (selectedAction_)
        {
        case 0:
            battle.playerAttack(0);
            break;
        case 1:
        {
            if (p && !p->skills().empty())
                battle.playerUseSkill(0, *p->skills()[0]);
            break;
        }
        case 2:
        {
            // Use the first usable recovery item as a free action.
            Inventory &inv = GameManager::instance().inventory();
            for (size_t i = 0; i < inv.size(); ++i)
            {
                Item *item = inv.itemAt(i);
                if (item && (item->type() == ItemType::Food || item->type() == ItemType::Potion ||
                             item->type() == ItemType::SpRecovery))
                {
                    if (battle.playerUseItem(item->clone()))
                        inv.removeItem(i);
                    break;
                }
            }
            break;
        }
        case 3:
        {
            // Cycle to next owned Persona for simplicity.
            const auto &owned = character.ownedPersonas();
            if (owned.size() > 1 && p)
            {
                size_t currentIdx = 0;
                for (size_t i = 0; i < owned.size(); ++i)
                {
                    if (owned[i] && owned[i]->id() == p->id())
                    {
                        currentIdx = i;
                        break;
                    }
                }
                size_t nextIdx = (currentIdx + 1) % owned.size();
                battle.playerSwitchPersona(owned[nextIdx]);
            }
            break;
        }
        case 4:
            battle.playerFlee();
            break;
        }
    }
}

void BattleScene::update(float /*deltaTime*/)
{
    // Enemy turns are processed synchronously inside the player action methods.
}

void BattleScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Reuse town background scaled to fill window with dark overlay.
    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 20, 200));

    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    Enemy *enemy = battle.enemyAt(0);

    // Enemy sprite
    if (enemy)
    {
        renderer.drawTexture("shadow", {520, 120, 96, 96});
        renderer.drawText(enemy->name(), {520, 80}, 22, engine::Color::white());
        renderer.drawText("HP " + std::to_string(enemy->hp()) + "/" + std::to_string(enemy->maxHp()),
                          {520, 260}, 18, engine::Color::red());
    }

    // Player sprite
    renderer.drawTexture("player", {150, 280, 64, 64});
    renderer.drawText(character.name(), {150, 260}, 22, engine::Color::white());
    renderer.drawText("HP " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()) +
                          "  SP " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()),
                      {150, 390}, 16, engine::Color::green());

    // Action menu panel
    renderer.drawRect({20, 420, 240, 160}, engine::Color(0, 0, 0, 200));
    const char *actions[] = {"Attack", "Skill", "Item", "Switch Persona", "Flee"};
    int actionCount = sizeof(actions) / sizeof(actions[0]);
    for (int i = 0; i < actionCount; ++i)
    {
        engine::Color color = (i == selectedAction_) ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(std::string((i == selectedAction_ ? "> " : "  ")) + actions[i],
                          {35.0f, 435.0f + i * 24.0f}, 18, color);
    }

    // Battle log panel
    renderer.drawRect({300, 420, 470, 160}, engine::Color(0, 0, 0, 200));
    renderer.drawText("Log:", {315, 435}, 18, engine::Color::white());
    const auto &log = battle.log();
    for (size_t i = 0; i < log.size() && i < 6; ++i)
    {
        renderer.drawText(log[log.size() - 1 - i], {315, 460.0f + static_cast<float>(i) * 19.0f}, 14, engine::Color::white());
    }

    if (battle.isOver())
    {
        std::string msg = battle.playerWon() ? "Victory! Press Enter." : "Defeat... Press Enter.";
        renderer.drawRect({200, 260, 400, 60}, engine::Color(0, 0, 0, 220));
        renderer.drawText(msg, {230, 280}, 24, engine::Color::yellow());
    }
}

bool BattleScene::processVictory()
{
    auto &battle = GameManager::instance().battleSystem();
    Enemy *enemy = battle.enemyAt(0);
    if (!enemy)
        return false;

    auto &character = GameManager::instance().character();
    int prevLevel = character.level();
    character.gainExp(enemy->rewardExp());
    character.addGold(enemy->rewardGold());

    // Direct Persona drops.
    for (const auto &pid : enemy->dropPersonaIds())
    {
        auto dropped = GameManager::instance().findPersona(pid);
        if (dropped)
        {
            GameManager::instance().addPersonaToPlayer(dropped);
            battle.appendLog("Obtained Persona: " + dropped->name());
        }
    }

    // If a level-up occurred, the snapshot will be populated by Character::levelUp().
    return character.hasLevelUpSnapshot();
}

void BattleScene::processDefeat()
{
    GameManager::instance().character().heal(GameManager::instance().character().maxHp());
}
