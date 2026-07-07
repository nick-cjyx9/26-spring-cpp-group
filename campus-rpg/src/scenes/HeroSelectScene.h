#pragma once

#include "IScene.h"

#include <string>
#include <vector>

// Hero selection screen shown when creating a new save.
// 4 heroes available, switch with Left/Right, type name, Enter to confirm.
class HeroSelectScene : public engine::IScene
{
public:
    HeroSelectScene();

    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    static constexpr int kHeroCount = 4;
    static constexpr float kFlashInterval = 0.5f;

    int selectedHero_ = 0;
    std::string nameBuffer_;
    std::string message_;
    float flashTimer_ = 0.0f;
    bool showCursor_ = true;

    void tryConfirm();
    void drawArrowLeft(engine::IRenderer &renderer, float cx, float cy, float size, engine::Color color) const;
    void drawArrowRight(engine::IRenderer &renderer, float cx, float cy, float size, engine::Color color) const;
};
