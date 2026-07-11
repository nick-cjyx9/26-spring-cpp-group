#pragma once

#include "IScene.h"

#include <string>
#include <vector>

class InventoryScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    std::vector<size_t> visibleItemIndices() const;

    int selectedIndex_ = 0;
    std::string message_;
    float messageTimer_ = 0.0f;
};
