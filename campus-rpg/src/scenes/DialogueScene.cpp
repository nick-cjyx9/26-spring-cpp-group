#include "DialogueScene.h"
#include "GameManager.h"
#include "SocialLinkManager.h"
#include "SocialLink.h"

void DialogueScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E) ||
        input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(SceneType::Town);
    }
}

void DialogueScene::update(float /*deltaTime*/)
{
    if (firstFrame_)
    {
        firstFrame_ = false;

        // Find nearest NPC to player for dialogue.
        TileMap &map = GameManager::instance().currentMap();
        Entity *player = nullptr;
        for (const auto &e : map.entities())
        {
            if (e && e->type() == "player")
            {
                player = e.get();
                break;
            }
        }

        if (player)
        {
            engine::Rect area{player->position().x - 24.0f, player->position().y - 24.0f, 48.0f, 48.0f};
            Entity *npc = map.firstEntityAt(area, "npc");
            if (npc)
                npcId_ = static_cast<NpcEntity *>(npc)->socialLinkId();
        }

        SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
        if (link)
        {
            link->addPoints(10);
        }
    }
}

void DialogueScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
    std::string name = link ? link->name() : "???";
    int rank = link ? link->rank() : 0;

    renderer.drawText("Talking with " + name, {10, 10}, 24, engine::Color::white());
    renderer.drawText("Social Link Rank: " + std::to_string(rank), {10, 50}, 18, engine::Color::cyan());
    renderer.drawText("\"Thanks for talking with me today!\"", {10, 120}, 18, engine::Color::white());
    renderer.drawText("Enter/Esc: continue", {10, 380}, 14, engine::Color::gray());
}
