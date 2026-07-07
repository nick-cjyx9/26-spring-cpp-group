#pragma once

#include "IScene.h"

class SocialLinkScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    void drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const;

    int selectedIndex_ = 0;
    float scrollY_ = 0.0f;
    static constexpr float kItemHeight = 120.0f;
    static constexpr float kScrollSmooth = 0.2f;
};
