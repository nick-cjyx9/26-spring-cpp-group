#include "PauseMenuScene.h"
#include "GameManager.h"

namespace
{
    // Rounded-look border: draw the outer frame plus a thin inner highlight.
    void drawPanel(engine::IRenderer &renderer, const engine::Rect &r,
                   engine::Color border, engine::Color inner)
    {
        const float t = 4.0f; // border thickness
        renderer.drawRect({r.x, r.y, r.width, t}, border);
        renderer.drawRect({r.x, r.y + r.height - t, r.width, t}, border);
        renderer.drawRect({r.x, r.y, t, r.height}, border);
        renderer.drawRect({r.x + r.width - t, r.y, t, r.height}, border);
        renderer.drawRect({r.x + t, r.y + t, r.width - 2 * t, r.height - 2 * t}, inner);
    }

    struct MenuItem
    {
        std::string label;
        engine::Rect box;
    };

    const MenuItem *menuItems()
    {
        static const MenuItem items[] = {
            {"exit",   {200, 175, 400, 90}},
            {"change", {200, 300, 400, 90}},
        };
        return items;
    }

    // Horizontally center `text` (at `size`) inside the box whose center x is cx.
    float centerX(const engine::IRenderer &renderer, const std::string &text,
                  int size, float cx)
    {
        return cx - renderer.textWidth(text, size) * 0.5f;
    }
}

void PauseMenuScene::handleInput(engine::IInput &input)
{
    const int kCount = 2;
    if (input.wasKeyJustPressed(engine::Key::Up))
        selectedIndex_ = (selectedIndex_ - 1 + kCount) % kCount;
    if (input.wasKeyJustPressed(engine::Key::Down))
        selectedIndex_ = (selectedIndex_ + 1) % kCount;

    if (input.wasKeyJustPressed(engine::Key::Enter) ||
        input.wasKeyJustPressed(engine::Key::E))
    {
        if (selectedIndex_ == 0)
        {
            // exit -> back to the Title (main menu) screen.
            GameManager::instance().enterScene(SceneType::Title);
        }
        else
        {
            // change -> open the save-slot browser in Load mode, same as
            // Title "load game". Loading a slot enters Town; Esc there goes
            // back to Title.
            GameManager::instance().openSaveSlots(false);
        }
        return;
    }

    if (input.wasKeyJustPressed(engine::Key::Escape))
    {
        // Cancel -> resume the game.
        GameManager::instance().enterScene(
            GameManager::instance().isNight() ? SceneType::Night : SceneType::Town);
    }
}

void PauseMenuScene::update(float /*deltaTime*/)
{
}

void PauseMenuScene::render(engine::IRenderer &renderer)
{
    renderer.clear();

    // Dim the game behind the menu (semi-transparent dark overlay).
    renderer.drawRect({0, 0, 800, 600}, engine::Color(10, 10, 25, 210));

    // Title bar.
    engine::Rect titleBar{250, 95, 300, 56};
    drawPanel(renderer, titleBar,
              engine::Color(120, 110, 200), engine::Color(40, 38, 70, 235));
    renderer.drawText("Paused",
                      {centerX(renderer, "Paused", 30, titleBar.x + titleBar.width / 2),
                       titleBar.y + 13},
                      30, engine::Color::white());

    const MenuItem *items = menuItems();
    for (int i = 0; i < 2; ++i)
    {
        bool selected = (i == selectedIndex_);
        engine::Color border = selected ? engine::Color::yellow()
                                        : engine::Color(90, 90, 120);
        engine::Color fill = selected ? engine::Color(55, 55, 90, 235)
                                      : engine::Color(30, 30, 50, 210);

        drawPanel(renderer, items[i].box, border, fill);

        std::string label = items[i].label;
        float cx = items[i].box.x + items[i].box.width / 2;
        renderer.drawText(label,
                          {centerX(renderer, label, 28, cx),
                           items[i].box.y + items[i].box.height / 2 - 16},
                          28,
                          selected ? engine::Color::yellow() : engine::Color::white());
    }

    renderer.drawText("Up/Down: select    Enter: confirm    Esc: resume",
                      {centerX(renderer, "Up/Down: select    Enter: confirm    Esc: resume",
                               16, 400.0f),
                       440},
                      16, engine::Color::gray());
}
