// LevelUpScene mock tests — verifies panel rendering and input dismissal.

#include "LevelUpScene.h"
#include "GameManager.h"
#include "Character.h"

#include "MockInput.h"
#include "MockRenderer.h"

#include <iostream>
#include <string>

namespace
{
    int g_run = 0;
    int g_failed = 0;

    void record(bool ok, const char *expr, const char *file, int line)
    {
        ++g_run;
        if (!ok)
        {
            ++g_failed;
            std::cerr << "FAIL: " << expr << "  (" << file << ':' << line << ")\n";
        }
    }

#define CHECK(expr) record(static_cast<bool>(expr), #expr, __FILE__, __LINE__)
#define CHECK_EQ(a, b) record((a) == (b), #a " == " #b, __FILE__, __LINE__)
} // namespace

void testLevelUpSceneRendersPanel()
{
    GameManager::instance().newGame("TestHero");
    auto &character = GameManager::instance().character();
    CHECK_EQ(character.level(), 1);
    character.gainExp(100); // triggers level up to 2
    CHECK(character.hasLevelUpSnapshot());
    CHECK_EQ(character.level(), 2);

    LevelUpScene scene;
    tests::MockRenderer renderer;
    scene.render(renderer);

    // Should draw the title.
    bool hasTitle = false;
    for (const auto &call : renderer.textCalls())
    {
        if (call.text == "LEVEL UP!")
            hasTitle = true;
    }
    CHECK(hasTitle);

    // Should draw at least one stat line with the old->new format.
    bool hasStatLine = false;
    for (const auto &call : renderer.textCalls())
    {
        if (call.text == "Max HP")
        {
            hasStatLine = true;
            break;
        }
    }
    CHECK(hasStatLine);

    // Should draw the hero portrait texture.
    bool hasPortrait = false;
    for (const auto &call : renderer.textureCalls())
    {
        if (call.textureId.find("hero_") != std::string::npos)
        {
            hasPortrait = true;
            break;
        }
    }
    CHECK(hasPortrait);
}

void testLevelUpSceneClosesOnEnter()
{
    GameManager::instance().newGame("TestHero");
    auto &character = GameManager::instance().character();
    character.gainExp(100);
    CHECK(character.hasLevelUpSnapshot());

    LevelUpScene scene;
    tests::MockInput input;
    input.setKeyPressed(engine::Key::Enter, true);
    scene.handleInput(input);
    CHECK(!character.hasLevelUpSnapshot());
}

void testLevelUpSceneClosesOnSpace()
{
    GameManager::instance().newGame("TestHero");
    auto &character = GameManager::instance().character();
    character.gainExp(100);
    CHECK(character.hasLevelUpSnapshot());

    LevelUpScene scene;
    tests::MockInput input;
    input.setKeyPressed(engine::Key::Space, true);
    scene.handleInput(input);
    CHECK(!character.hasLevelUpSnapshot());
}

int main()
{
    testLevelUpSceneRendersPanel();
    testLevelUpSceneClosesOnEnter();
    testLevelUpSceneClosesOnSpace();

    std::cout << "\n==== CampusRPG level-up scene tests ====\n";
    std::cout << "run: " << g_run << "  failed: " << g_failed << "\n";
    std::cout << (g_failed == 0 ? "ALL PASSED\n" : "THERE ARE FAILURES\n");
    return g_failed == 0 ? 0 : 1;
}
