#pragma once

#include "IScene.h"

#include <string>
#include <vector>

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
    void drawResultOverlay(engine::IRenderer &renderer) const;
    void handleMainMenu(engine::IInput &input);
    void handleSkillMenu(engine::IInput &input);
    void handleItemMenu(engine::IInput &input);
    void handlePersonaMenu(engine::IInput &input);
    void returnAfterBattle();
    void beginSleepTransition();
    void finishSleepTransition();

    BattleMenu menu_ = BattleMenu::Main;
    int selectedAction_ = 0;
    int selectedSkill_ = 0;
    int selectedItem_ = 0;
    int selectedPersona_ = 0;
    std::string message_;
    bool sleepTransition_ = false;
    float sleepTimer_ = 0.0f;
    bool sleepLeveledUp_ = false;
    bool sleepStartedOnSecondMap_ = false;
    bool postBattleHandled_ = false;
    bool showingResult_ = false;
    bool resultVictory_ = false;
    int resultExp_ = 0;
    int resultGold_ = 0;
    std::vector<std::string> resultPersonas_;
};
