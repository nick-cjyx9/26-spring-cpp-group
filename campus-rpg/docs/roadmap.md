# 扩展计划（Roadmap）

## 已实现 / 稳定

- [x] 四层架构与 CMake 库划分。
- [x] 抽象引擎接口 `IRenderer`/`IWindow`/`IInput` + SFML/Mock 实现。
- [x] 核心域模型：`Character`、`Item`、`Inventory`、`Enemy`、`Shop`、`BattleSystem`、`Quest`。
- [x] P4 简化机制设计：`Persona`、`SocialLink`、夜晚城镇战斗。
- [x] 场景系统：`TitleScene`、`TownScene`、`NightScene`、`BattleScene`、`ShopScene`、`InventoryScene`、`CharacterScene`、`DialogueScene`。
- [x] 游戏循环与场景切换安全（避免悬空指针）。
- [x] SFML 纹理/字体资源加载与构建后自动复制。
- [x] 标题画面可进入城镇，各子界面可返回。

## 近期（Checkpoint 3 前必须完成）

- [x] 抽象引擎接口代码实现。
- [x] SFML 渲染器、窗口、输入实现。
- [x] Mock 实现（供测试）。
- [x] 游戏循环 `GameLoop`。
- [x] 新 Core 类：`Persona`、`SocialLink`、`SocialLinkManager`、`Entity`、`TileMap`。
- [x] 简化 `BattleSystem`（去掉 1 More / 总攻击）。
- [x] `GameManager` 接入场景系统。
- [x] `TitleScene` / `TownScene` / `NightScene` / `BattleScene` / `ShopScene` / `InventoryScene` / `CharacterScene` / `DialogueScene`。
- [x] SQLite schema 扩展与 `SaveRepository` 更新。
- [x] Core 层单元测试 + Mock 场景测试。
- [ ] UI 美术资源精调与瓦片地图正式化。
- [ ] 更多任务条件：`collect:id:N`、`level:N`。
- [ ] 战斗掉落与任务进度联动。

## 中期（Checkpoint 4 前）

- [x] 美术资源接入：纹理、瓦片地图、精灵动画（初版）。
- [ ] 敌人 AI 策略多样化。
- [ ] 更多 Persona 和敌人种类。
- [ ] 音效与背景音乐。

## 远期（可选）

- [ ] Persona 合成系统。
- [ ] 装备槽系统（武器/防具/饰品）。
- [ ] 多存档槽位。
- [ ] 数据统计与可视化。
