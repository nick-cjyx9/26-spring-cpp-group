#include "ShopScene.h"
#include "GameManager.h"
#include "Shop.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>

void ShopScene::handleInput(engine::IInput &input)
{
    auto &gm = GameManager::instance();
    auto &shop = gm.shop();
    auto &inventory = gm.inventory();

    if (input.wasKeyJustPressed(engine::Key::Left) || input.wasKeyJustPressed(engine::Key::A) ||
        input.wasKeyJustPressed(engine::Key::Right) || input.wasKeyJustPressed(engine::Key::D))
    {
        mode_ = (mode_ == Mode::Buy) ? Mode::Sell : Mode::Buy;
        selectedIndex_ = 0;
        sellIndex_ = 0;
        message_.clear();
        return;
    }

    if (mode_ == Mode::Buy)
    {
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
            Item *item = shop.items()[static_cast<size_t>(selectedIndex_)].get();
            auto *personaItem = dynamic_cast<PersonaItem *>(item);
            if (personaItem)
            {
                if (gm.character().gold() < item->value())
                {
                    message_ = "Not enough gold.";
                }
                else
                {
                    auto persona = gm.findPersona(personaItem->personaId());
                    if (!persona)
                    {
                        message_ = "Unknown Persona.";
                    }
                    else
                    {
                        gm.character().spendGold(item->value());
                        gm.addPersonaToPlayer(persona);
                        message_ = "Obtained Persona: " + persona->name();
                    }
                }
            }
            else if (shop.buy(static_cast<size_t>(selectedIndex_), gm.character(), inventory))
            {
                message_ = "Purchased!";
            }
            else
            {
                message_ = inventory.isFull() ? "Backpack is full." : "Not enough gold.";
            }
        }
    }
    else
    {
        int count = static_cast<int>(inventory.size());
        if (count > 0)
        {
            if (input.wasKeyJustPressed(engine::Key::Up))
                sellIndex_ = (sellIndex_ - 1 + count) % count;
            if (input.wasKeyJustPressed(engine::Key::Down))
                sellIndex_ = (sellIndex_ + 1) % count;
        }

        if ((input.wasKeyJustPressed(engine::Key::Enter) || input.wasKeyJustPressed(engine::Key::E)) && count > 0)
        {
            Item *item = inventory.itemAt(static_cast<size_t>(sellIndex_));
            std::string itemName = item ? item->name() : "item";
            if (shop.sell(static_cast<size_t>(sellIndex_), gm.character(), inventory))
            {
                message_ = "Sold: " + itemName;
                if (sellIndex_ >= static_cast<int>(inventory.size()))
                    sellIndex_ = std::max(0, static_cast<int>(inventory.size()) - 1);
            }
            else
            {
                message_ = "Cannot sell item.";
            }
        }
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        gm.enterScene(gm.isNight() ? SceneType::Night : SceneType::Town);
    }
}

void ShopScene::update(float /*deltaTime*/)
{
}

void ShopScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    auto &gm = GameManager::instance();
    auto &inventory = gm.inventory();

    renderer.drawTexture("town_bg", {0, 0, 800, 600});
    renderer.drawRect({0, 0, 800, 600}, engine::Color(0, 0, 0, 160));

    renderer.drawRect({110, 45, 580, 510}, engine::Color(30, 30, 50, 235));
    renderer.drawText("Shop", {135, 65}, 28, engine::Color::white());
    renderer.drawText("Gold: " + std::to_string(gm.character().gold()) +
                          "   Backpack: " + std::to_string(inventory.size()) + "/" + std::to_string(inventory.capacity()),
                      {135, 105}, 18, engine::Color::yellow());

    engine::Color buyColor = (mode_ == Mode::Buy) ? engine::Color::yellow() : engine::Color::gray();
    engine::Color sellColor = (mode_ == Mode::Sell) ? engine::Color::yellow() : engine::Color::gray();
    renderer.drawText("Buy", {135, 135}, 20, buyColor);
    renderer.drawText("Sell", {205, 135}, 20, sellColor);
    renderer.drawText("A/D or Left/Right: switch", {430, 138}, 14, engine::Color::gray());

    if (mode_ == Mode::Buy)
    {
        const auto &items = gm.shop().items();
        for (size_t i = 0; i < items.size(); ++i)
        {
            engine::Color color = (static_cast<int>(i) == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
            std::string prefix = (static_cast<int>(i) == selectedIndex_) ? "> " : "  ";
            std::string type = items[i]->typeString();
            renderer.drawText(prefix + items[i]->name() + " [" + type + "] - " + std::to_string(items[i]->value()) + "G",
                              {135.0f, 175.0f + static_cast<float>(i) * 42.0f}, 18, color);
            renderer.drawText("  " + items[i]->description(), {155.0f, 197.0f + static_cast<float>(i) * 42.0f}, 13, engine::Color::gray());
        }
        renderer.drawText("Enter: buy   Esc: back", {135, 525}, 14, engine::Color::gray());
    }
    else
    {
        const auto &items = inventory.items();
        if (items.empty())
        {
            renderer.drawText("Backpack is empty.", {135, 185}, 18, engine::Color::gray());
        }
        else
        {
            for (size_t i = 0; i < items.size(); ++i)
            {
                engine::Color color = (static_cast<int>(i) == sellIndex_) ? engine::Color::yellow() : engine::Color::white();
                std::string prefix = (static_cast<int>(i) == sellIndex_) ? "> " : "  ";
                int sellPrice = items[i]->value() / 2;
                renderer.drawText(prefix + items[i]->name() + " [" + items[i]->typeString() + "] - " + std::to_string(sellPrice) + "G",
                                  {135.0f, 175.0f + static_cast<float>(i) * 30.0f}, 18, color);
            }
        }
        renderer.drawText("Enter: sell   Esc: back", {135, 525}, 14, engine::Color::gray());
    }

    renderer.drawText(message_, {135, 490}, 16, engine::Color::red());
}
