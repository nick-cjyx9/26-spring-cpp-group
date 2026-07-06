#include "InventoryScene.h"
#include "GameManager.h"
#include "Inventory.h"
#include "Item.h"

void InventoryScene::handleInput(engine::IInput &input)
{
    auto &inventory = GameManager::instance().inventory();
    size_t count = inventory.size();

    if (count > 0)
    {
        if (input.wasKeyJustPressed(engine::Key::Up))
            selectedIndex_ = (selectedIndex_ - 1 + static_cast<int>(count)) % static_cast<int>(count);
        if (input.wasKeyJustPressed(engine::Key::Down))
            selectedIndex_ = (selectedIndex_ + 1) % static_cast<int>(count);
    }

    if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) && count > 0)
    {
        if (inventory.useItem(static_cast<size_t>(selectedIndex_), GameManager::instance().character()))
            message_ = "Item used.";
        else
            message_ = "Cannot use item.";
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        GameManager::instance().enterScene(GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void InventoryScene::update(float /*deltaTime*/)
{
}

void InventoryScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({150, 50, 500, 500}, engine::Color(30, 30, 50, 230));
    renderer.drawText("Inventory", {170, 70}, 28, engine::Color::white());

    const auto &inventory = GameManager::instance().inventory();
    const auto &items = inventory.items();
    renderer.drawText("Capacity: " + std::to_string(items.size()) + "/" + std::to_string(inventory.capacity()),
                      {170, 110}, 16, engine::Color::white());

    for (size_t i = 0; i < items.size(); ++i)
    {
        engine::Color color = (static_cast<int>(i) == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
        std::string prefix = (static_cast<int>(i) == selectedIndex_) ? "> " : "  ";
        renderer.drawText(prefix + items[i]->name() + " [" + items[i]->typeString() + "]",
                          {170.0f, 145.0f + static_cast<float>(i) * 28.0f}, 20, color);
    }

    renderer.drawText(message_, {170, 480}, 16, engine::Color::red());
    renderer.drawText("Enter: use, Esc: back", {170, 520}, 14, engine::Color::gray());
}
