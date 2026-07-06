#include "ShopScene.h"
#include "GameManager.h"
#include "Shop.h"
#include "Item.h"

void ShopScene::handleInput(engine::IInput &input)
{
    auto &shop = GameManager::instance().shop();
    size_t count = shop.items().size();

    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(count)) % static_cast<int>(count);
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(count);

    if (input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E))
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
    renderer.drawText("Shop", {10, 10}, 24, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(GameManager::instance().character().gold()), {10, 45}, 18, engine::Color::yellow());

    const auto &items = GameManager::instance().shop().items();
    for (size_t i = 0; i < items.size(); ++i)
    {
        engine::Color color = (static_cast<int>(i) == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(items[i]->name() + " - " + std::to_string(items[i]->value()) + "G",
                          {10.0f, 80.0f + static_cast<float>(i) * 25.0f}, 18, color);
    }

    renderer.drawText(message_, {10, 350}, 16, engine::Color::red());
    renderer.drawText("Enter: buy, Esc: back", {10, 380}, 14, engine::Color::gray());
}
