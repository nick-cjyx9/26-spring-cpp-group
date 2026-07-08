#pragma once

#include "IScene.h"
#include <string>

// Level-up panel shown when a character levels up.
// Displays the hero portrait on the left and stat changes on the right.
// Press Enter/Space to dismiss and return to the previous scene.
class LevelUpScene : public engine::IScene
{
public:
    explicit LevelUpScene(std::string returnTextureId = "town_bg");

    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    std::string returnTextureId_;

    void drawStatLine(engine::IRenderer &renderer, const std::string &label,
                        int oldVal, int newVal, float x, float y, int size) const;
};
