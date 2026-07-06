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
    renderer.drawText("Inventory", {10, 10}, 24, engine::Color::white());

    const auto &inventory = GameManager::instance().inventory();
    const auto &items = inventory.items();
    renderer.drawText("Capacity: " + std::to_string(items.size()) + "/" + std::to_string(inventory.capacity()), {10, 45}, 16, engine::Color::white());

    for (size_t i = 0; i < items.size(); ++i)
    {
        engine::Color color = (static_cast<int>(i) == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
        renderer.drawText(items[i]->name() + " [" + items[i]->typeString() + "]",
                          {10.0f, 80.0f + static_cast<float>(i) * 25.0f}, 18, color);
    }

    renderer.drawText(message_, {10, 350}, 16, engine::Color::red());
    renderer.drawText("Enter: use, Esc: back", {10, 380}, 14, engine::Color::gray());
}
