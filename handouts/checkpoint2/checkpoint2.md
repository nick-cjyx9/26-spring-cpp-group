# 检查点 2：类图设计与模块进度

## 1. 系统 UML 类图

![CampusRPG 2D 系统 UML 类图](./UML类图.svg)

> 源文件：`handouts/checkpoint2/UML类图.mmd`，已由 Mermaid.ink 重新渲染为 SVG/PNG。

### 设计要点说明

- **四层架构**：Engine（抽象接口 + SFML/Mock 实现）→ GameManager（单例总控）→ Core（纯 C++ 领域模型）+ Data（SQLite3 持久化）。
- **面向对象特征**：
  - 封装：`Character`、`Quest` 等类私有字段 + 公开 getter/setter。
  - 继承：抽象基类 `Item`、`Enemy`、`Entity`、`Scene`、`IRenderer`、`IWindow`、`IInput`。
  - 多态：`item->use()`、`enemy->battleCry()`、`renderer->drawRect()`、`scene->update()` 运行时绑定。
  - 关联：`GameManager` 组合 `Character`、`Inventory`、`Shop`、`QuestManager`、`SocialLinkManager`、`TileMap`。
- **STL 应用**：
  - `std::vector<std::unique_ptr<Item>>` 管理背包/商店物品。
  - `std::map<std::string, Quest>` 管理任务。
  - `std::map<std::string, SocialLink>` 管理社群。
  - `std::vector<std::unique_ptr<Enemy>>` 保存敌人模板。
  - `std::vector<std::string>` 记录战斗日志。
- **可测试性**：引擎层通过 `IRenderer`/`IWindow`/`IInput` 抽象接口与 SFML 解耦；`CampusRPGCore` 纯 C++ 静态库供单元测试链接；测试使用 Mock 实现验证场景行为。

## 2. 代码模块完成情况

**已完成设计（检查点 2）**：
- 最终玩法设计：早晨 Social Link + 夜晚同地图战斗 + Persona + 商店/背包。
- 新四层架构与 CMake 库划分。
- 抽象引擎接口 `IRenderer` / `IWindow` / `IInput` 及 SFML/Mock 实现方案。
- Core 域模型类图：`Character`、`Persona`、`Skill`、`Item` 体系、`Enemy` 体系、`BattleSystem`、`Shop`、`Quest`、`SocialLink`、`Entity`、`TileMap`。
- Data 层 SQLite schema 扩展设计。
- 用户界面改为 SFML 2D 场景系统：`TownScene`、`NightScene`、`BattleScene`、`ShopScene`、`InventoryScene`、`DialogueScene`。

**开发中模块（检查点 3 前完成）**：
- SFML 引擎具体实现。
- 场景代码实现。
- SQLite3 持久化代码迁移（从 QtSql 改为 SQLite3 C API 或继续保留 QtSql 待决定）。
- 单元测试与 Mock 场景测试。

**已完成的软件工程/挑战任务（规划）**：
- 数据库技术：SQLite3 持久化。
- 图形用户界面技术：SFML 2D 图形界面。
- STL 高级应用：`map`、`vector`、`algorithm` 综合使用。
- 软件工程实践：Git 协同、单元测试、自动化构建、持续集成。

**遇到的问题**：
- 原 Qt Widgets 方案不适合 2D 地图移动，改为 SFML 引擎。
- 为支持无图形库的单元测试，引入 `IRenderer`/`IWindow`/`IInput` 抽象接口与 Mock 实现。

---

**字数**：约 520 字
