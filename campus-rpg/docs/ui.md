# 用户界面（UI）

`src/engine/sfml/` 与 `src/scenes/` 基于 SFML 实现 2D 图形界面。UI 只负责渲染与转发输入，不包含业务逻辑。

## 技术栈

- SFML 2.6+
- 2D 顶视角地图
- 场景（Scene）制界面切换

## 页面/场景清单

| 场景 | 文件 | 职责 |
| --- | --- | --- |
| `TitleScene` | `src/scenes/TitleScene.*` | 标题画面，开始游戏 / 读取游戏 / 退出 |
| `TownScene` | `src/scenes/TownScene.*` | 早晨城镇/学校地图，角色移动、NPC 对话、进入功能 |
| `NightScene` | `src/scenes/NightScene.*` | 夜晚同地图，敌人游荡，碰撞触发战斗 |
| `BattleScene` | `src/scenes/BattleScene.*` | 简化回合制战斗界面 |
| `ShopScene` | `src/scenes/ShopScene.*` | 商店买卖 |
| `InventoryScene` | `src/scenes/InventoryScene.*` | 背包查看、使用/丢弃道具 |
| `CharacterScene` | `src/scenes/CharacterScene.*` | 角色属性、当前 Persona、装备 |
| `DialogueScene` | `src/scenes/DialogueScene.*` | Social Link 对话展示 |

## 设计原则

- 每个场景实现 `IScene` 接口：`handleInput`、`update`、`render`。
- 所有业务操作通过 `GameManager::instance()` 完成。
- 场景不直接访问 `DatabaseManager` / `SaveRepository`。
- 2D 资源使用 `resources/textures/` 下的 PNG 精灵 + 几何图形 + 文字；运行时自动复制到构建目录。

## 导航流程

```text
GameLoop
 ├── TitleScene（标题菜单）
 │    └── TownScene（开始游戏）
 ├── TownScene（早晨）
 │    ├── DialogueScene（与 NPC 对话）
 │    ├── ShopScene
 │    ├── InventoryScene / CharacterScene
 │    └── NightScene（按 N 进入夜晚）
 ├── NightScene（夜晚）
 │    └── BattleScene（碰到敌人）
 └── BattleScene
      └── TownScene（战斗结束）
```

## 输入约定

| 按键 | 功能 |
| --- | --- |
| ↑/↓ / Enter | 标题菜单选择 |
| WASD / 方向键 | 角色移动 |
| E / Enter | 交互（对话、进门） |
| I | 打开背包 |
| C | 打开角色面板 |
| N | 从城镇进入夜晚 |
| Esc | 返回/关闭面板 |

## 错误提示

- 金币不足购买：屏幕下方显示红色提示文字。
- 背包已满：提示“背包已满”。
- 无法使用道具（如 HP 满时使用回血道具）：提示“无需使用”。
- 逃跑失败：提示“逃跑失败”。
