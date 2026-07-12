#include "ShopScene.h"
#include "GameManager.h"
#include "Shop.h"
#include "Item.h"
#include "Persona.h"

#include <algorithm>

namespace
{
    constexpr int kVisibleShopRows = 7;
    constexpr float kListTop = 175.0f;
    constexpr float kRowHeight = 42.0f;

    int firstVisibleIndex(int selectedIndex, int itemCount)
    {
        if (itemCount <= kVisibleShopRows)
            return 0;
        int first = selectedIndex - kVisibleShopRows / 2;
        first = std::max(0, first);
        return std::min(first, itemCount - kVisibleShopRows);
    }

    std::string texturePathForShopItem(const Item &item)
    {
        const std::string &textureId = item.textureId();
        if (textureId.empty())
            return "";
        if (textureId.find('/') != std::string::npos || textureId.rfind("arcana_", 0) == 0)
            return textureId;
        return std::string("equipment/") + textureId;
    }

    void drawScrollbar(engine::IRenderer &renderer, int selectedIndex, int itemCount)
    {
        if (itemCount <= kVisibleShopRows)
            return;

        const float trackX = 660.0f;
        const float trackY = kListTop;
        const float trackH = kVisibleShopRows * kRowHeight - 10.0f;
        renderer.drawRect({trackX, trackY, 6.0f, trackH}, engine::Color(60, 60, 80, 220));

        float thumbH = std::max(24.0f, trackH * static_cast<float>(kVisibleShopRows) / static_cast<float>(itemCount));
        float maxThumbY = trackY + trackH - thumbH;
        float t = itemCount <= 1 ? 0.0f : static_cast<float>(selectedIndex) / static_cast<float>(itemCount - 1);
        renderer.drawRect({trackX, trackY + (maxThumbY - trackY) * t, 6.0f, thumbH}, engine::Color(220, 220, 110, 240));
    }
} // namespace

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
                        if (gm.addPersonaToPlayer(persona))
                        {
                            gm.character().spendGold(item->value());
                            message_ = "Obtained Persona: " + persona->name();
                        }
                        else
                        {
                            message_ = "Persona slots full or already owned.";
                        }
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
        int itemCount = static_cast<int>(items.size());
        int first = firstVisibleIndex(selectedIndex_, itemCount);
        int last = std::min(first + kVisibleShopRows, itemCount);
        for (int i = first; i < last; ++i)
        {
            engine::Color color = (i == selectedIndex_) ? engine::Color::yellow() : engine::Color::white();
            std::string prefix = (i == selectedIndex_) ? "> " : "  ";
            std::string type = items[static_cast<size_t>(i)]->typeString();
            float y = kListTop + static_cast<float>(i - first) * kRowHeight;

            std::string texPath = texturePathForShopItem(*items[static_cast<size_t>(i)]);
            if (!texPath.empty())
                renderer.drawTexture(texPath, {155.0f, y, 32, 32});
            else
                renderer.drawRect({155.0f, y, 32, 32}, engine::Color(60, 60, 80));

            renderer.drawText(prefix + items[static_cast<size_t>(i)]->name() + " [" + type + "] - " + std::to_string(items[static_cast<size_t>(i)]->value()) + "G",
                              {195.0f, y + 6}, 18, color);
            renderer.drawText("  " + items[static_cast<size_t>(i)]->description(), {195.0f, y + 24}, 13, engine::Color::gray());
        }
        drawScrollbar(renderer, selectedIndex_, itemCount);
        if (itemCount > 0)
            renderer.drawText("Showing " + std::to_string(first + 1) + "-" + std::to_string(last) + " / " + std::to_string(itemCount),
                              {500, 525}, 14, engine::Color::gray());
        renderer.drawText("Up/Down: scroll   Enter: buy   Esc: back", {135, 525}, 14, engine::Color::gray());
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
            int itemCount = static_cast<int>(items.size());
            int first = firstVisibleIndex(sellIndex_, itemCount);
            int last = std::min(first + kVisibleShopRows, itemCount);
            for (int i = first; i < last; ++i)
            {
                engine::Color color = (i == sellIndex_) ? engine::Color::yellow() : engine::Color::white();
                std::string prefix = (i == sellIndex_) ? "> " : "  ";
                int sellPrice = items[static_cast<size_t>(i)]->value() / 2;
                float y = kListTop + static_cast<float>(i - first) * kRowHeight;

                std::string texPath = texturePathForShopItem(*items[static_cast<size_t>(i)]);
                if (!texPath.empty())
                    renderer.drawTexture(texPath, {155.0f, y, 32, 32});
                else
                    renderer.drawRect({155.0f, y, 32, 32}, engine::Color(60, 60, 80));

                renderer.drawText(prefix + items[static_cast<size_t>(i)]->name() + " [" + items[static_cast<size_t>(i)]->typeString() + "] - " + std::to_string(sellPrice) + "G",
                                  {195.0f, y + 6}, 18, color);
            }
            drawScrollbar(renderer, sellIndex_, itemCount);
            renderer.drawText("Showing " + std::to_string(first + 1) + "-" + std::to_string(last) + " / " + std::to_string(itemCount),
                              {500, 525}, 14, engine::Color::gray());
        }
        renderer.drawText("Enter: sell   Esc: back", {135, 525}, 14, engine::Color::gray());
    }

    renderer.drawText(message_, {135, 490}, 16, engine::Color::red());
}
