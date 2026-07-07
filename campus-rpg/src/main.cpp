#include "GameManager.h"
#include "DatabaseManager.h"
#include "SfmlWindow.h"

#include <SFML/System/Clock.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <iostream>
#include <memory>

int main()
{
    if (!DatabaseManager::instance().initDatabase("campus_rpg.db"))
    {
        std::cerr << "Failed to initialize database.\n";
        return 1;
    }

    GameManager::instance().newGame("Player");

    // ---- 奶龙 Rank-Up 音效（占位，用户可替换 resources/sounds/nailong_rankup.wav） ----
    auto rankUpSoundBuffer = std::make_shared<sf::SoundBuffer>();
    auto rankUpSound = std::make_shared<sf::Sound>();
    bool rankUpSoundLoaded = false;
    if (rankUpSoundBuffer->loadFromFile("resources/sounds/nailong_rankup.wav"))
    {
        rankUpSound->setBuffer(*rankUpSoundBuffer);
        rankUpSoundLoaded = true;
    }

    GameManager::instance().setRankUpCallback(
        [rankUpSound, rankUpSoundLoaded](const std::string & /*socialLinkId*/, int /*newRank*/)
    {
        if (rankUpSoundLoaded && rankUpSound)
        {
            rankUpSound->stop();
            rankUpSound->play();
        }
    });

    engine::SfmlWindow window(800, 600, "Campus RPG");
    engine::SfmlInput input;

    sf::Clock clock;
    while (window.isOpen() && !GameManager::instance().shouldQuit())
    {
        window.pollEvents(input);

        float dt = clock.restart().asSeconds();

        if (auto *scene = GameManager::instance().currentScene())
            scene->handleInput(input);
        if (auto *scene = GameManager::instance().currentScene())
            scene->update(dt);
        if (auto *scene = GameManager::instance().currentScene())
            scene->render(window.renderer());
        window.renderer().present();
        input.endFrame();
    }

    return 0;
}
