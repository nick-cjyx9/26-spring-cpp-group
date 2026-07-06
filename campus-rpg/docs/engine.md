# 引擎层设计（Engine Layer）

引擎层负责窗口管理、事件循环、输入处理和 2D 渲染。为了支持无图形库的单元测试，引擎能力被抽象为接口，上层代码只依赖接口，不依赖 SFML。

## 抽象接口

### IRenderer

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void clear() = 0;
    virtual void present() = 0;
    virtual void drawRect(const Rect& rect, Color color) = 0;
    virtual void drawTexture(const std::string& textureId, const Vec2& pos) = 0;
    virtual void drawTexture(const std::string& textureId, const Rect& dstRect) = 0;
    virtual void drawText(const std::string& text, const Vec2& pos, int size, Color color) = 0;
};
```

职责：所有绘制请求的入口。上层场景通过 `IRenderer` 绘制矩形、纹理、文字。

### IWindow

```cpp
class IWindow {
public:
    virtual ~IWindow() = default;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    virtual void pollEvents(IInput& input) = 0;
    virtual IRenderer& renderer() = 0;
};
```

职责：窗口生命周期管理、事件分发。

### IInput

```cpp
class IInput {
public:
    virtual ~IInput() = default;
    virtual bool isKeyPressed(Key key) const = 0;
    virtual bool wasKeyJustPressed(Key key) = 0;
    virtual void endFrame() = 0;
};
```

职责：键盘输入查询。`wasKeyJustPressed` 用于交互键（如对话、开门），避免一帧内重复触发。

## SFML 实现

- `SfmlRenderer`：封装 `sf::RenderWindow`、资源管理（`sf::Texture`、`sf::Font`）。
- `SfmlWindow`：创建窗口、事件循环、关联 renderer。
- `SfmlInput`：将 `sf::Keyboard` 状态映射到 `Key` 枚举。

## Mock 实现

测试用 Mock 不创建真实窗口，只记录调用：

- `MockRenderer`：记录 `drawRect`/`drawTexture`/`drawText` 的调用次数和参数。
- `MockWindow`：模拟窗口是否打开。
- `MockInput`：允许测试代码预设按键状态。

Mock 实现放在 `tests/mocks/`，只被 `CampusRPGTests` 链接。

## 游戏循环 GameLoop

```cpp
while (window.isOpen() && !GameManager::instance().shouldQuit()) {
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
```

注意：`handleInput` 可能切换场景并销毁旧场景对象，因此必须在每次调用前重新获取当前场景指针，避免悬空指针崩溃。

## 场景系统

```cpp
class IScene {
public:
    virtual ~IScene() = default;
    virtual void handleInput(IInput& input) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(IRenderer& renderer) = 0;
};
```

具体场景：
- `TitleScene`：标题画面与主菜单。
- `TownScene`：早晨城镇/学校移动、NPC 对话、进入商店、等到夜晚。
- `NightScene`：夜晚同地图，敌人游荡，碰撞触发战斗。
- `BattleScene`：简化回合制战斗界面。
- `ShopScene`：商店买卖界面。
- `InventoryScene`：背包/角色状态界面。
- `CharacterScene`：角色属性与 Persona 展示。
- `DialogueScene`：对话展示。

`GameManager` 持有 `std::unique_ptr<IScene> currentScene_` 并负责切换。
