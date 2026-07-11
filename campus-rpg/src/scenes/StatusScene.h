#pragma once

#include "IScene.h"

#include <string>
#include <vector>

// Character page.
// Left: carried Persona slots (max 6) for out-of-battle view/switch/destroy.
// Right: equipment page (equipped slots + equipment items from backpack only).
class StatusScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class Section
    {
        Persona,
        Equipment
    };

    void handlePersonaInput(engine::IInput &input);
    void handleEquipmentInput(engine::IInput &input);

    void renderProfilePanel(engine::IRenderer &renderer);
    void renderPersonaPanel(engine::IRenderer &renderer);
    void renderEquipmentPanel(engine::IRenderer &renderer);
    void renderSelectedPersonaDetail(engine::IRenderer &renderer, float x, float y);
    void renderSkillDetailsOverlay(engine::IRenderer &renderer);

    std::vector<size_t> equipmentInventoryIndices() const;

    Section section_ = Section::Persona;
    int personaIndex_ = 0;
    int equipmentIndex_ = 0; // 0..3 = equipped slots, 4+ = backpack equipment rows
    bool showSkillDetails_ = false;
    std::string message_;
    float messageTimer_ = 0.0f;
};
