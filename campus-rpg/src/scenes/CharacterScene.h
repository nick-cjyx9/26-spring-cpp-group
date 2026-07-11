#pragma once

#include "IScene.h"

class CharacterScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    float animationTime_ = 0.0f;
};
