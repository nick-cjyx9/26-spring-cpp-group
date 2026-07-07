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
            // Each battle costs 4 in-game hours.
            GameManager::instance().advanceTime(GameManager::kBattleCostHours);
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
    const char *actions[] = {"Attack", "Skill", "Item", "Guard", "Switch Persona", "Flee"};
    for (int i = 0; i < 6; ++i)
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

void BattleScene::processVictory()
{
    auto &battle = GameManager::instance().battleSystem();
    Enemy *enemy = battle.enemyAt(0);
    if (!enemy)
        return;

    auto &character = GameManager::instance().character();
    character.gainExp(enemy->rewardExp());
    character.addGold(enemy->rewardGold());
}

void BattleScene::processDefeat()
{
    GameManager::instance().character().heal(GameManager::instance().character().maxHp());
}
