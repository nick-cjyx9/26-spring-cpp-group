#pragma once

#include "IScene.h"
#include <string>

class GameCompleteScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    float timer_ = 0.0f;
    bool showContinue_ = false;
};