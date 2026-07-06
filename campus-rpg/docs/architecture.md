# 总体架构

## 分层设计

```text
┌─────────────────────────────────────┐
│ Engine / Rendering Layer            │
│  IRenderer / IWindow / IInput       │
│  SFML 实现 / Mock 实现               │
└──────────────┬──────────────────────┘
               │ 转发玩家输入、渲染请求
┌──────────────▼──────────────────────┐
│ Manager Layer                       │
│  GameManager（单例）                  │
│  状态协调、场景切换、存档入口           │
└──────────────┬──────────────────────┘
               │
    ┌──────────┴──────────┐
    ▼                     ▼
┌──────────────┐   ┌──────────────┐
│ Core         │   │ Data         │
│ 纯 C++ 域模型       SQLite3 持久化   │
└──────────────┘   └──────────────┘
```

## 静态库划分

| 库 | 源码 | 依赖 | 说明 |
| --- | --- | --- | --- |
| `CampusRPGCore` | `src/core/` | 纯 C++ | 领域模型，供单元测试直接链接 |
| `CampusRPGAppLib` | `src/data/` + `src/manager/` + `src/engine/interfaces/` | `CampusRPGCore` + SQLite3 | 持久化、状态管理、引擎抽象接口 |
| `CampusRPG` | `src/engine/sfml/` + `src/main.cpp` | `CampusRPGAppLib` + SFML | 可执行程序 |
| `CampusRPGTests` | `tests/test_core.cpp` + `tests/mocks/` | `CampusRPGCore` + Mock 实现 | 单元测试，无真实图形库依赖 |

## OOP 课程检查点

- **封装**：所有实体类使用 private 字段 + public getter/setter。
- **继承**：抽象基类 `Item`、`Enemy`、`Entity`、`Scene`，引擎抽象接口 `IRenderer`、`IWindow`、`IInput`。
- **多态**：`item->use(character)`、`enemy->battleCry()`、`renderer->drawRect()`、`scene->update()` 运行时绑定。
- **关联**：`GameManager` 组合 `Character`、`Inventory`、`Shop`、`SocialLinkManager`、`TileMap`。

## STL 课程检查点

- `std::vector<std::unique_ptr<Item>>`：库存、商店商品、敌人列表。
- `std::map<std::string, Quest>`：任务查找。
- `std::map<std::string, SocialLink>`：社群管理。
- `std::vector<std::string>`：战斗日志。

## 抽象基类与 Mock

为支持无图形库的单元测试，引擎层通过抽象接口与上层解耦：

- `IRenderer`：绘制接口，SFML 与 Mock 分别实现。
- `IWindow`：窗口管理接口。
- `IInput`：输入接口。

Core 层保持纯 C++，不依赖 SFML/SQLite，确保测试快速、可移植。
