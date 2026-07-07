#pragma once

#include "IScene.h"

#include <string>

class DialogueScene : public engine::IScene
{
public:
    void handleInput(engine::IInput &input) override;
    void update(float deltaTime) override;
    void render(engine::IRenderer &renderer) override;

private:
    std::string npcId_;
    std::string dialogueText_;
    bool firstFrame_ = true;

    // Rank-up banner state
    bool showRankUpBanner_ = false;
    float rankUpTimer_ = 0.0f;
    static constexpr float kRankUpDuration = 2.5f;

    // NPC texture
    std::string npcTexId_;
    bool hasNpcTex_ = false;

    // Hero (protagonist) texture + name
    std::string heroTexId_;
    bool hasHeroTex_ = false;
    std::string playerName_;

    // Draws 5 heart slots representing the bond rank.
    void drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const;
};
