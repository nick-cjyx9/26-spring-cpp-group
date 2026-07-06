# 检查点 2：类图设计与模块进度

## 1. 系统 UML 类图

![CampusRPG 系统 UML 类图](./UML类图.svg)

> 源文件：`handouts/checkpoint2/UML类图.mmd`，可由 Mermaid CLI 重新渲染为 SVG/PNG。

### 设计要点说明

- **四层架构**：UI（Qt Widgets）→ GameManager（单例总控）→ Core（纯 C++ 领域模型）+ Data（SQLite 持久化）。
- **面向对象特征**：
  - 封装：`Character`、`Quest` 等类私有字段 + 公开 getter/setter。
  - 继承：抽象基类 `Item` 派生 `FoodItem`/`PotionItem`/`EquipmentItem`；抽象基类 `Enemy` 派生 `Slime`/`Goblin`/`Boss`。
  - 多态：`item->use(character)`、`enemy->battleCry()` 运行时绑定。
  - 关联：`GameManager` 组合 `Character`/`Inventory`/`Shop`/`QuestManager`；`BattleSystem` 使用 `Character` 与 `Enemy`。
- **STL 应用**：`std::vector<std::unique_ptr<Item>>` 管理背包/商店物品，`std::map<std::string, Quest>` 管理任务，`std::vector<std::string>` 记录战斗日志，`std::unique_ptr<Enemy>` 保存敌人模板。
- **可测试性**：`core/` 层无 Qt/SQLite 依赖，已单独编译为 `CampusRPGCore` 静态库供单元测试链接。

## 2. 代码模块完成情况

**已完成模块**：
- `Character`（角色属性、等级成长、金币、经验、装备加成）
- `Item` 体系及三种具体物品（食物/药水/装备）
- `Inventory` 背包（增删用）与 `Shop` 商店（买卖）
- `Quest`/`QuestManager` 任务系统（接取/完成/领奖）
- `Enemy` 敌人类及 `Slime`/`Goblin`/`Boss` 三个派生类
- `BattleSystem` 回合制战斗与日志
- `GameManager` 单例、默认数据初始化
- `DatabaseManager`/`SaveRepository` SQLite 持久化
- Qt Widgets UI：主窗口 + 角色/背包/商店/任务/战斗页面

**开发中模块**：
- 任务条件目前只支持 `kill:N`，后续扩展 `collect:id:N`、`level:N` 等。
- 装备目前是永久加成，后续增加真实装备槽与卸下机制。

**已完成的软件工程/挑战任务**：
- 数据库（SQLite）+ GUI（Qt）+ 软件工程实践（Git/单元测试/自动化构建/CI）。

**遇到的问题**：
在 VS Code 中 Qt 头文件（如 `QApplication`）曾因 CMake/MinGW 把 Qt include 路径写入响应文件（`@includes_CXX.rsp`）而无法被 IntelliSense 解析；已通过提交共享 `.vscode/` 配置并改用显式 `includePath` 解决。

---

**字数**：约 290 字
