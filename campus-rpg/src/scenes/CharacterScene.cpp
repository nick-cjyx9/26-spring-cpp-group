#include "CharacterScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Persona.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    std::string arcanaTextureId(std::string arcana)
    {
        std::transform(arcana.begin(), arcana.end(), arcana.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::replace(arcana.begin(), arcana.end(), ' ', '_');
        return "arcana_" + arcana;
    }

    void drawNeonFrame(engine::IRenderer &renderer, const engine::Rect &rect, engine::Color color)
    {
        renderer.drawRect({rect.x - 10, rect.y - 10, rect.width + 20, rect.height + 20}, engine::Color(color.r, color.g, color.b, 28));
        renderer.drawRect({rect.x - 5, rect.y - 5, rect.width + 10, rect.height + 10}, engine::Color(color.r, color.g, color.b, 45));
        renderer.drawRect({rect.x - 2, rect.y - 2, rect.width + 4, 3}, color);
        renderer.drawRect({rect.x - 2, rect.y + rect.height - 1, rect.width + 4, 3}, color);
        renderer.drawRect({rect.x - 2, rect.y - 2, 3, rect.height + 4}, color);
        renderer.drawRect({rect.x + rect.width - 1, rect.y - 2, 3, rect.height + 4}, color);
    }
}

void CharacterScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void CharacterScene::update(float deltaTime)
{
    animationTime_ += deltaTime;
}

void CharacterScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({70, 45, 660, 510}, engine::Color(18, 18, 38, 235));

    const auto &character = GameManager::instance().character();
    Persona *p = character.currentPersona();

    float pulse = (std::sin(animationTime_ * 3.0f) + 1.0f) * 0.5f;
    auto cyanGlow = engine::Color(60, 220, 255, static_cast<unsigned char>(120 + pulse * 90));
    auto goldGlow = engine::Color(255, 220, 90, static_cast<unsigned char>(110 + pulse * 100));

    renderer.drawText("Character", {330, 70}, 28, engine::Color::white());

    // Left: equipped Persona's Arcana card, with a simple animated neon aura.
    engine::Rect card{105.0f, 125.0f, 190.0f, 285.0f};
    drawNeonFrame(renderer, card, cyanGlow);
    renderer.drawRect({card.x + 8, card.y + 8, card.width - 16, card.height - 16}, engine::Color(8, 8, 24, 230));

    for (int i = 0; i < 4; ++i)
    {
        float offset = std::fmod(animationTime_ * 45.0f + i * 56.0f, card.height + 80.0f) - 40.0f;
        renderer.drawRect({card.x - 18.0f + i * 62.0f, card.y + offset, 38.0f, 2.0f}, engine::Color(80, 230, 255, 145));
    }
    for (int i = 0; i < 10; ++i)
    {
        float phase = animationTime_ * 1.7f + static_cast<float>(i) * 0.75f;
        float x = 200.0f + std::cos(phase) * (118.0f + static_cast<float>(i % 3) * 8.0f);
        float y = 267.0f + std::sin(phase * 1.35f) * (168.0f + static_cast<float>(i % 2) * 10.0f);
        renderer.drawRect({x, y, 4.0f, 4.0f}, engine::Color(160, 245, 255, 170));
    }

    if (p)
    {
        renderer.drawTexture(arcanaTextureId(p->arcana()), {card.x + 20, card.y + 25, card.width - 40, card.height - 50});
        renderer.drawText(p->arcana(), {card.x + 32, card.y + card.height + 18}, 20, goldGlow);
        renderer.drawText(p->name() + "  Lv" + std::to_string(p->level()), {card.x + 5, card.y + card.height + 48}, 18, engine::Color::cyan());
    }
    else
    {
        renderer.drawText("No Persona", {card.x + 38, card.y + 130}, 20, engine::Color::gray());
    }

    // Right: character stats.
    float infoX = 335.0f;
    renderer.drawRect({infoX - 18, 118, 360, 310}, engine::Color(25, 25, 55, 210));
    drawNeonFrame(renderer, {infoX - 18, 118, 360, 310}, goldGlow);
    renderer.drawText("Name: " + character.name(), {infoX, 135}, 20, engine::Color::white());
    renderer.drawText("Level: " + std::to_string(character.level()), {infoX, 168}, 20, engine::Color::white());
    renderer.drawText("HP: " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()), {infoX, 201}, 20, engine::Color::green());
    renderer.drawText("SP: " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()), {infoX, 234}, 20, engine::Color::cyan());
    renderer.drawText("EXP: " + std::to_string(character.exp()) + "/" + std::to_string(character.expToNextLevel()), {infoX, 267}, 20, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(character.gold()), {infoX, 300}, 20, engine::Color::yellow());

    renderer.drawText("Final Stats", {infoX, 348}, 18, goldGlow);
    renderer.drawText("STR " + std::to_string(character.attack()) +
                          "   MAG " + std::to_string(character.magic()) +
                          "   SPD " + std::to_string(character.speed()),
                      {infoX, 378}, 18, engine::Color::white());

    renderer.drawText("Esc: back", {330, 520}, 14, engine::Color::gray());
}
