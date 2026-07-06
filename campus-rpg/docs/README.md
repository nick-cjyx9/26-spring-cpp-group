# Campus RPG 设计文档

本文档描述 Campus RPG 2D 校园 RPG 冒险游戏系统的设计。

## 目录

1. [总体架构](architecture.md)
2. [核心域模型](core.md)（Character / Persona / Item / Inventory / Enemy / BattleSystem / Shop / SocialLink / TileMap）
3. [引擎层设计](engine.md)（IRenderer / IWindow / IInput / SFML 实现 / Mock 实现）
4. [用户界面](ui.md)（SFML 2D 场景系统）
5. [数据持久化](data.md)（SQLite3 / SaveRepository / DatabaseManager）
6. [游戏管理器](manager.md)（GameManager 单例）
7. [扩展计划](roadmap.md)
8. [简化版 P4 玩法设计](persona4_design.md)

## 项目定位

面向 2026 春季“程序设计课程设计大作业”的 C++/SFML/SQLite 课程项目，重点展示面向对象设计、STL 使用、数据库持久化、抽象基类 + Mock 测试、2D 图形界面与软件工程实践（Git、CI、单元测试）。
