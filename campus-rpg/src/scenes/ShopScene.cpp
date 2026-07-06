#include "ShopScene.h"
#include "GameManager.h"
#include "Shop.h"
#include "Item.h"

void ShopScene::handleInput(engine::IInput &input)
{
    auto &shop = GameManager::instance().shop();
    size_t count = shop.items().size();

    if (count > 0)
    {
        if (input.wasKeyJustPressed(engine::Key::Up))
            selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(count)) % static_cast<int>(count);
        if (input.wasKeyJustPressed(engine::Key::Down))
            selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(count);
    }

    if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) && count > 0)
    {
        if (shop.buy(static_cast<size_t>(selectedIndex_), GameManager::instance().character(), GameManager::instance().inventory()))
            message_ = "Purchased!";
        else
            message_ = "Cannot buy (not enough gold or full inventory).";
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void ShopScene::update(float /*deltaTime*/)
{
}

void ShopScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({150, 50, 500, 500}, engine::Color(30, 30, 50, 230));
    renderer.drawText("Shop", {170, 70}, 28, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(GameManager::instance().character().gold()), {170, 110}, 18, engine::Color::yellow());

    const auto &items = GameManager::instance().shop().items();
    for (size_t i = 0; i < items.size(); ++i)
    {
        engine::Color color = (static_cast<int>(i) == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
        std::string prefix = (static_cast<int>(i) == selectedIndex_) ? "> " : "  ";
        renderer.drawText(prefix + items[i]->name() + " - " + std::to_string(items[i]->value()) + "G",
                          {170.0f, 155.0f + static_cast<float>(i) * 30.0f}, 20, color);
        renderer.drawText("  " + items[i]->description(), {190.0f, 175.0f + static_cast<float>(i) * 30.0f}, 14, engine::Color::gray());
    }

    renderer.drawText(message_, {170, 480}, 16, engine::Color::red());
    renderer.drawText("Enter: buy, Esc: back", {170, 520}, 14, engine::Color::gray());
}
