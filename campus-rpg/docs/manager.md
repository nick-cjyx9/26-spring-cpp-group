# 游戏管理器（GameManager）

`src/manager/GameManager` 是单例，作为游戏状态的唯一真相来源（single source of truth）。

## 职责

- 持有并管理：
  - `Character player_`
  - `Inventory inventory_`
  - `Shop shop_`
  - `QuestManager questManager_`
  - `SocialLinkManager socialLinkManager_`
  - `TileMap currentMap_`
  - `BattleSystem battleSystem_`
- 协调场景切换。
- 负责 `newGame()` 的默认数据初始化。
- 提供 `save()` / `load()` 入口，委托给 `SaveRepository`。

## 单例访问

```cpp
GameManager& GameManager::instance();
```

## 关键生命周期

1. **启动/新建游戏**：`newGame()` 初始化默认商店、任务、敌人、Persona、Social Link 数据。
2. **读取存档**：在 `newGame()` 之后调用 `load()`，确保基础定义已存在。
3. **保存**：`save()` 委托 `SaveRepository`。

## 场景切换

```cpp
void enterScene(SceneType type);
```

`GameManager` 持有 `std::unique_ptr<IScene> currentScene_`，根据游戏阶段切换：

- `TitleScene`：标题菜单
- `TownScene`：早晨
- `NightScene`：夜晚
- `BattleScene`：战斗
- `ShopScene` / `InventoryScene` / `CharacterScene`：功能界面
- `DialogueScene`：对话

同时提供 `shouldQuit()` / `quit()`，用于场景请求退出程序（如标题画面选择 exit）。

## 当前功能

- 创建默认玩家并装备起始 Persona。
- 初始化默认商店商品。
- 初始化默认任务、敌人、Social Link。
- 保存/读取角色、背包、Persona、Social Link、任务进度。
