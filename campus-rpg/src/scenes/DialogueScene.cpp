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

        // Resolve texture ids once per scene entry.
        if (!npcId_.empty())
        {
            avatarTexId_ = "npc_" + npcId_ + "_avatar";
            fullTexId_ = "npc_" + npcId_ + "_full";
            hasTextures_ = true;
        }

        int beforeRank = 0;
        if (const SocialLink *before = GameManager::instance().socialLinkManager().getLink(npcId_))
            beforeRank = before->rank();

        if (!npcId_.empty())
            dialogueText_ = GameManager::instance().talkToNpc(npcId_);

        const SocialLink *after = GameManager::instance().socialLinkManager().getLink(npcId_);
        if (after && after->rank() > beforeRank)
        {
            showRankUpBanner_ = true;
            rankUpTimer_ = kRankUpDuration;
        }
    }

    if (showRankUpBanner_)
    {
        rankUpTimer_ -= 0.016f; // approximate per-frame delta
        if (rankUpTimer_ <= 0.0f)
            showRankUpBanner_ = false;
    }
}

void DialogueScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    SocialLink *link = GameManager::instance().socialLinkManager().getLink(npcId_);
    std::string name = link ? link->name() : "???";
    int rank = link ? link->rank() : 0;

    // ---- Town background ----
    renderer.drawTexture("town_bg", {0, 0, 800, 600});

    // ---- Bottom dialogue box ----
    renderer.drawRect({50, 400, 700, 150}, engine::Color(20, 20, 40, 230));
    renderer.drawText("Talking with " + name + "  (Rank " + std::to_string(rank) + ")",
                      {70, 415}, 22, engine::Color::cyan());
    renderer.drawText(dialogueText_.empty() ? "\"...\"" : dialogueText_, {70, 460}, 20, engine::Color::white());
    renderer.drawText("Enter/Esc: continue", {70, 520}, 14, engine::Color::gray());

    // ---- Top-left: small NPC avatar (tou xiang) ----
    if (hasTextures_)
    {
        renderer.drawTexture(avatarTexId_, {20, 20, 80, 80});
    }
    else
    {
        // Fallback: colored rectangle with name
        renderer.drawRect({20, 20, 80, 80}, engine::Color(60, 40, 80, 200));
        renderer.drawText(name, {30, 45}, 14, engine::Color::white());
    }

    // ---- Bottom-right: full NPC sprite (li hui) ----
    // Draw slightly higher to make room for hearts below
    if (hasTextures_)
    {
        renderer.drawTexture(fullTexId_, {500, 180, 280, 340});
    }
    else
    {
        // Fallback: larger colored rectangle
        renderer.drawRect({500, 180, 280, 340}, engine::Color(40, 40, 60, 180));
        renderer.drawText(name + "\nFull Art", {550, 330}, 18, engine::Color::white());
    }

    // ---- Hearts under NPC sprite ----
    drawHearts(renderer, 500.0f, 530.0f, rank);

    // ---- Bottom-left: protagonist mini-sprite (ren wu li hui) ----
    {
        renderer.drawRect({20, 300, 120, 90}, engine::Color(50, 50, 70, 180));
        renderer.drawText("Protagonist", {30, 325}, 14, engine::Color::white());
    }

    // ---- Center-top: Rank-Up banner ----
    if (showRankUpBanner_)
    {
        // Semi-transparent dark overlay behind banner
        renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 120));

        // Banner box
        renderer.drawRect({200, 50, 400, 80}, engine::Color(200, 50, 50, 220));
        renderer.drawText("RANK UP!!", {280, 65}, 36, engine::Color(255, 255, 0, 255));
        renderer.drawText(name + " reached Rank " + std::to_string(rank) + "!",
                          {240, 110}, 20, engine::Color::white());

        // Flash effect: bright border
        unsigned char alpha = static_cast<unsigned char>(rankUpTimer_ / kRankUpDuration * 255);
        renderer.drawRect({198, 48, 404, 84}, engine::Color(255, 200, 0, alpha));
    }
}

void DialogueScene::drawHearts(engine::IRenderer &renderer, float x, float y, int rank) const
{
    // Map rank 0..10 to hearts 0..5 (each heart = 2 ranks)
    int fullHearts = rank / 2;
    bool hasHalf = (rank % 2) != 0;

    for (int i = 0; i < 5; ++i)
    {
        float heartX = x + static_cast<float>(i) * 26.0f;
        if (i < fullHearts)
        {
            // Full heart - bright red
            renderer.drawRect({heartX, y, 20.0f, 20.0f}, engine::Color(255, 50, 50, 230));
        }
        else if (i == fullHearts && hasHalf)
        {
            // Half heart - left half red, right half gray
            renderer.drawRect({heartX, y, 10.0f, 20.0f}, engine::Color(255, 50, 50, 230));
            renderer.drawRect({heartX + 10.0f, y, 10.0f, 20.0f}, engine::Color(100, 100, 100, 150));
        }
        else
        {
            // Empty heart - dark gray
            renderer.drawRect({heartX, y, 20.0f, 20.0f}, engine::Color(100, 100, 100, 150));
        }
    }
}
