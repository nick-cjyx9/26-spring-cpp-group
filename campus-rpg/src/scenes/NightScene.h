#pragma once

#include "IScene.h"

class NightScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    float moveSpeed_ = 150.0f;
    float moveX_ = 0.0f;
    float moveY_ = 0.0f;
    std::string stuckMessage_;
    float stuckMessageTimer_ = 0.0f;

    // Prevent rapid re-interaction after exiting dialogue.
    float interactionCooldown_ = 0.0f;
    static constexpr float kInteractionCooldown = 0.3f;
};
