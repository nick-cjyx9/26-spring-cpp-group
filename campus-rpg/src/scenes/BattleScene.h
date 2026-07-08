#pragma once

#include "IScene.h"

class BattleScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    enum class BattleMenu
    {
        Main,
        Skill,
        Item,
        Persona
    };

    bool processVictory();
    void processDefeat();
    void handleMainMenu(engine::IInput &input);
    void handleSkillMenu(engine::IInput &input);
    void handleItemMenu(engine::IInput &input);
    void handlePersonaMenu(engine::IInput &input);
    void returnAfterBattle();

    BattleMenu menu_ = BattleMenu::Main;
    int selectedAction_ = 0;
    int selectedSkill_ = 0;
    int selectedItem_ = 0;
    int selectedPersona_ = 0;
    std::string message_;
};
