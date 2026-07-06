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

    const auto &character = GameManager::instance().character();
    renderer.drawText("Character", {10, 10}, 24, engine::Color::white());
    renderer.drawText("Name: " + character.name(), {10, 50}, 18, engine::Color::white());
    renderer.drawText("Level: " + std::to_string(character.level()), {10, 75}, 18, engine::Color::white());
    renderer.drawText("HP: " + std::to_string(character.hp()) + "/" + std::to_string(character.maxHp()), {10, 100}, 18, engine::Color::white());
    renderer.drawText("SP: " + std::to_string(character.sp()) + "/" + std::to_string(character.maxSp()), {10, 125}, 18, engine::Color::white());
    renderer.drawText("EXP: " + std::to_string(character.exp()) + "/" + std::to_string(character.expToNextLevel()), {10, 150}, 18, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(character.gold()), {10, 175}, 18, engine::Color::yellow());

    renderer.drawText("ATK: " + std::to_string(character.attack()) + "  DEF: " + std::to_string(character.defense()) +
                          "  SPD: " + std::to_string(character.speed()) + "  MAG: " + std::to_string(character.magic()),
                      {10, 210}, 18, engine::Color::white());

    Persona *p = character.currentPersona();
    if (p)
    {
        renderer.drawText("Persona: " + p->name() + " (" + p->arcana() + ") Lv" + std::to_string(p->level()),
                          {10, 250}, 18, engine::Color::cyan());
    }

    renderer.drawText("Esc: back", {10, 380}, 14, engine::Color::gray());
}
