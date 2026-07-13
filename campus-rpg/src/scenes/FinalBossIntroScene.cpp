#include "FinalBossIntroScene.h"
#include "GameManager.h"
#include "BattleSystem.h"
#include "Enemy.h"

void FinalBossIntroScene::handleInput(engine::IInput &input)
{
    if (messageTimer_ > 0.0f)
    {
        if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
        {
            messageTimer_ = 0.0f;
            message_.clear();
        }
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
    {
        if (currentDialogueIndex_ + 1 < static_cast<int>(dialogues_.size()))
        {
            ++currentDialogueIndex_;
        }
        else
        {
            // Start final boss battle
            auto enemy = GameManager::instance().createEnemyFromTemplate("enemy_final_boss");
            if (enemy)
            {
                GameManager::instance().setCurrentEnemyTextureId("boss_pixel");
                GameManager::instance().battleSystem().startBattle(GameManager::instance().character(), *enemy);
                GameManager::instance().enterScene(SceneType::Battle);
            }
            else
            {
                GameManager::instance().enterScene(SceneType::Town);
            }
        }
    }
}

void FinalBossIntroScene::update(float deltaTime)
{
    if (messageTimer_ > 0.0f)
    {
        messageTimer_ -= deltaTime;
        if (messageTimer_ <= 0.0f)
        {
            messageTimer_ = 0.0f;
            message_.clear();
        }
    }
}

void FinalBossIntroScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Dark background
    renderer.drawRect({0, 0, 800, 600}, engine::Color(10, 0, 20, 255));

    // Boss portrait (centered, large)
    renderer.drawTexture("boss_portrait", {250, 50, 300, 300});

    // Dialogue box
    renderer.drawRect({50, 380, 700, 180}, engine::Color(20, 10, 30, 230));
    renderer.drawRect({52, 382, 696, 176}, engine::Color(40, 20, 60, 200));

    // Boss name tag
    renderer.drawRect({70, 360, 200, 30}, engine::Color(80, 0, 120, 230));
    renderer.drawText("Shadow Overlord", {80, 365}, 18, engine::Color(255, 100, 255, 255));

    // Dialogue text
    renderer.drawText(dialogues_[currentDialogueIndex_], {80, 420}, 20, engine::Color::white());

    // Continue hint
    renderer.drawText("Press Enter to continue...", {80, 520}, 16, engine::Color(180, 180, 180, 255));

    // Message overlay
    if (!message_.empty())
    {
        renderer.drawRect({200, 250, 400, 100}, engine::Color(20, 20, 30, 230));
        renderer.drawRect({202, 252, 396, 96}, engine::Color(40, 20, 60, 200));
        renderer.drawText(message_, {220, 290}, 20, engine::Color::yellow());
    }
}