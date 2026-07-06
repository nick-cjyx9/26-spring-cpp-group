#include "GameManager.h"
#include "DatabaseManager.h"
#include "SfmlWindow.h"

#include <SFML/System/Clock.hpp>

#include <iostream>

int main()
{
    if (!DatabaseManager::instance().initDatabase("campus_rpg.db"))
    {
        std::cerr << "Failed to initialize database.\n";
        return 1;
    }

    GameManager::instance().newGame("Player");

    engine::SfmlWindow window(800, 600, "Campus RPG");
    engine::SfmlInput input;

    sf::Clock clock;
    while (window.isOpen())
    {
        window.pollEvents(input);

        float dt = clock.restart().asSeconds();

        auto *scene = GameManager::instance().currentScene();
        if (scene)
        {
            scene->handleInput(input);
            scene->update(dt);
            scene->render(window.renderer());
        }
    }

    return 0;
}
