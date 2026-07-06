#include "BattleScene.h"
#include "GameManager.h"
#include "BattleSystem.h"
#include "Enemy.h"
#include "Persona.h"

void BattleScene::handleInput(engine::IInput &input)
{
    if (GameManager::instance().battleSystem().isOver())
    {
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::Escape))
        {
            if (GameManager::instance().battleSystem().playerWon())
                processVictory();
            else
                processDefeat();
            GameManager::instance().setNight(false);
            GameManager::instance().enterScene(SceneType::Town);
        }
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedAction_ = (selectedAction_ + 5) % 6;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedAction_ = (selectedAction_ + 1) % 6;

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        auto &battle = GameManager::instance().battleSystem();
        auto &character = GameManager::instance().character();

        switch (selectedAction_)
        {
        case 0:
            battle.playerAttack(0);
            break;
        case 1:
        {
            Persona *p = character.currentPersona();
            if (p && !p->skills().empty())
                battle.playerUseSkill(0, *p->skills()[0]);
            break;
        }
        case 2:
            battle.playerUseItem(0);
            break;
        case 3:
            battle.playerGuard();
            break;
        case 4:
        {
            auto pixie = GameManager::instance().findPersona("persona_pixie");
            if (pixie)
                battle.playerSwitchPersona(pixie);
            break;
        }
        case 5:
            battle.playerFlee();
            break;
        }
    }
}

void BattleScene::update(float /*deltaTime*/)
{
}

void BattleScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    auto &battle = GameManager::instance().battleSystem();
    auto &character = GameManager::instance().character();
    Enemy *enemy = battle.enemyAt(0);

    renderer.drawText("Battle", {10, 10}, 24, engine::Color::white());

    if (enemy)
    {
        renderer.drawText(enemy->name() + " HP: " + std::to_string(enemy->hp()) + "/" + std::to_string(enemy->maxHp()),
                          {10, 50}, 18, engine::Color::red());
    }

    renderer.drawText(character.name() + " HP: " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()) +
                          " SP: " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()),
                      {10, 90}, 18, engine::Color::green());

    const char *actions[] = {"Attack", "Skill", "Item", "Guard", "Switch Persona", "Flee"};
    for (int i = 0; i < 6; ++i)
    {
        engine::Color color = (i == selectedAction_) ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(std::string((i == selectedAction_ ? "> " : "  ")) + actions[i], {10.0f, 130.0f + i * 25.0f}, 18, color);
    }

    renderer.drawText("Log:", {300, 130}, 18, engine::Color::white());
    const auto &log = battle.log();
    for (size_t i = 0; i < log.size() && i < 10; ++i)
    {
        renderer.drawText(log[log.size() - 1 - i], {300, 155.0f + static_cast<float>(i) * 20.0f}, 14, engine::Color::white());
    }

    if (battle.isOver())
    {
        std::string msg = battle.playerWon() ? "Victory! Press Enter." : "Defeat... Press Enter.";
        renderer.drawText(msg, {10, 320}, 20, engine::Color::yellow());
    }
}

void BattleScene::processVictory()
{
    auto &battle = GameManager::instance().battleSystem();
    Enemy *enemy = battle.enemyAt(0);
    if (!enemy)
        return;

    auto &character = GameManager::instance().character();
    character.gainExp(enemy->rewardExp());
    character.addGold(enemy->rewardGold());

    // Simple quest progress.
    GameManager::instance().questManager().completeQuest("quest_first", 1);
}

void BattleScene::processDefeat()
{
    // Reset player HP on defeat.
    GameManager::instance().character().heal(GameManager::instance().character().maxHp());
}
