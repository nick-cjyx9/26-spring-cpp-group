#pragma once

#include "IScene.h"

class RestConfirmScene : public engine::IScene
{
public:
    explicit RestConfirmScene(bool nightMode = false) : nightMode_(nightMode) {}
    ~RestConfirmScene() override;
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    void confirm(); // enter night (day mode) or advance to next day (night mode)

    void drawBorder(engine::IRenderer &r, float x, float y, float w, float h,
                    engine::Color bright, engine::Color dark);
    void drawShadow(engine::IRenderer &r, float x, float y, float w, float h, float offset = 4.0f);
    void drawPanel(engine::IRenderer &r, float x, float y, float w, float h,
                   engine::Color fill, engine::Color bright, engine::Color dark);

    int selected_ = 0; // 0 = confirm, 1 = cancel
    bool nightMode_ = false;
};