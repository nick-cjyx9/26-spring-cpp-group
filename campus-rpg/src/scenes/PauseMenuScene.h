#pragma once

#include "IScene.h"

// In-game pause menu, reached by pressing Esc from the Town scene.
// Presents two centered options:
//   - exit:     return to the Title (main menu) screen.
//   - exchange: switch to another save slot (same flow as Title "load game").
// Pressing Esc again cancels and resumes the game (back to Town/Night).
class PauseMenuScene : public engine::IScene
{
public:
    PauseMenuScene() = default;

    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    int selectedIndex_ = 0; // 0 = exit, 1 = exchange
};
