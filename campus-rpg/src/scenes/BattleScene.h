#pragma once

#include "IScene.h"

class BattleScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    void processVictory();
    void processDefeat();

    int selectedAction_ = 0;
};
