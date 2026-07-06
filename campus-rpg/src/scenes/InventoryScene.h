#pragma once

#include "IScene.h"

class InventoryScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    int selectedIndex_ = 0;
    std::string message_;
};
