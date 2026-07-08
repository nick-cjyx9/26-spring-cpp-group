#pragma once

#include "IScene.h"

#include <memory>
#include <string>
#include <vector>

// Status / Equipment / Backpack panel.
// Left: Stats    | Middle: Gear (4 slots)    | Right: Backpack
class StatusScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class Section
    {
        Stats,
        Gear,
        Backpack
    };

    void handleStatsInput(engine::IInput &input);
    void handleGearInput(engine::IInput &input);
    void handleBackpackInput(engine::IInput &input);

    void renderStats(engine::IRenderer &renderer);
    void renderGear(engine::IRenderer &renderer);
    void renderBackpack(engine::IRenderer &renderer);

    Section section_ = Section::Stats;
    int gearSlotIndex_ = 0; // 0=Weapon, 1=Armor, 2=Accessory, 3=Relic
    int backpackIndex_ = 0;
    std::string message_;
    float messageTimer_ = 0.0f;
};
