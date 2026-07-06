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
            engine::Rect area{player->position().x - 28.0f, player->position().y - 28.0f, 56.0f, 56.0f};
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

    // Town background
    renderer.drawTexture("town_bg", {0, 0, 800, 600});

    SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
    std::string name = link ? link->name() : "???";
    int rank = link ? link->rank() : 0;

    // Dialogue box
    renderer.drawRect({50, 400, 700, 150}, engine::Color(20, 20, 40, 230));
    renderer.drawText("Talking with " + name + "  (Rank " + std::to_string(rank) + ")",
                      {70, 415}, 22, engine::Color::cyan());
    renderer.drawText("\"Thanks for talking with me today!\"", {70, 460}, 20, engine::Color::white());
    renderer.drawText("Enter/Esc: continue", {70, 520}, 14, engine::Color::gray());
}
