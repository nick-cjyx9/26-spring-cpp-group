# 扩展计划（Roadmap）

## 已实现 / 稳定

- [x] 四层架构与 CMake 库划分。
- [x] 抽象引擎接口 `IRenderer`/`IWindow`/`IInput` + SFML/Mock 实现设计。
- [x] 核心域模型：`Character`、`Item`、`Inventory`、`Enemy`、`Shop`、`BattleSystem`、`Quest`。
- [x] P4 简化机制设计：`Persona`、`SocialLink`、夜晚城镇战斗。

## 近期（Checkpoint 3 前必须完成）

- [ ] 抽象引擎接口代码实现。
- [ ] SFML 渲染器、窗口、输入实现。
- [ ] Mock 实现（供测试）。
- [ ] 游戏循环 `GameLoop`。
- [ ] 新 Core 类：`Persona`、`SocialLink`、`SocialLinkManager`、`Entity`、`TileMap`。
- [ ] 简化 `BattleSystem`（去掉 1 More / 总攻击）。
- [ ] `GameManager` 接入场景系统。
- [ ] `TownScene` / `NightScene` / `BattleScene` / `ShopScene` / `InventoryScene` / `CharacterScene` / `DialogueScene`。
- [ ] SQLite schema 扩展与 `SaveRepository` 更新。
- [ ] Core 层单元测试 + Mock 场景测试。

## 中期（Checkpoint 4 前）

- [ ] 美术资源接入：纹理、瓦片地图、精灵动画。
- [ ] 敌人 AI 策略多样化。
- [ ] 任务条件扩展：`collect:id:N`、`level:N`。
- [ ] 更多 Persona 和敌人种类。
- [ ] 音效与背景音乐。

## 远期（可选）

- [ ] Persona 合成系统。
- [ ] 装备槽系统（武器/防具/饰品）。
- [ ] 多存档槽位。
- [ ] 数据统计与可视化。
