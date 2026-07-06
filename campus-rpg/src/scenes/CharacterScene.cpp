#include "CharacterScene.h"
#include "GameManager.h"
#include "Character.h"
#include "Persona.h"

void CharacterScene::handleInput(engine::IInput &input)
{
    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void CharacterScene::update(float /*deltaTime*/)
{
}

void CharacterScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({150, 50, 500, 500}, engine::Color(30, 30, 50, 230));

    const auto &character = GameManager::instance().character();
    renderer.drawText("Character", {170, 70}, 28, engine::Color::white());
    renderer.drawText("Name: " + character.name(), {170, 120}, 20, engine::Color::white());
    renderer.drawText("Level: " + std::to_string(character.level()), {170, 150}, 20, engine::Color::white());
    renderer.drawText("HP: " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()), {170, 180}, 20, engine::Color::white());
    renderer.drawText("SP: " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()), {170, 210}, 20, engine::Color::white());
    renderer.drawText("EXP: " + std::to_string(character.exp()) + "/" + std::to_string(character.expToNextLevel()), {170, 240}, 20, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(character.gold()), {170, 270}, 20, engine::Color::yellow());

    renderer.drawText("ATK: " + std::to_string(character.attack()) + "  DEF: " + std::to_string(character.defense()) +
                          "  SPD: " + std::to_string(character.speed()) + "  MAG: " + std::to_string(character.magic()),
                      {170, 320}, 18, engine::Color::white());

    Persona *p = character.currentPersona();
    if (p)
    {
        renderer.drawText("Persona: " + p->name() + " (" + p->arcana() + ") Lv" + std::to_string(p->level()),
                          {170, 370}, 20, engine::Color::cyan());
    }

    renderer.drawText("Esc: back", {170, 520}, 14, engine::Color::gray());
}
