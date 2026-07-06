# 数据持久化（Data）

`src/data/` 负责 SQLite3 数据库的创建、连接管理与保存/读取，UI 与 Core 不直接依赖数据库细节。

## 依赖

- SQLite3（C API 或通过 `find_package(SQLite3)` / `FetchContent`）
- 不再依赖 QtSql

## DatabaseManager（数据库连接单例）

### 职责
- 创建并管理唯一的 SQLite3 数据库连接。
- 初始化数据表（若不存在则创建）。
- 提供数据库连接与错误处理接口。

### 当前表结构

```sql
CREATE TABLE IF NOT EXISTS character (
    id INTEGER PRIMARY KEY CHECK(id = 1),
    name TEXT,
    level INTEGER,
    hp INTEGER, max_hp INTEGER,
    sp INTEGER, max_sp INTEGER,
    exp INTEGER, exp_to_next INTEGER,
    gold INTEGER,
    pos_x REAL, pos_y REAL,
    is_night INTEGER,
    current_persona_id TEXT
);

CREATE TABLE IF NOT EXISTS persona (
    id TEXT PRIMARY KEY,
    name TEXT,
    arcana TEXT,
    level INTEGER,
    st INTEGER, ma INTEGER, en INTEGER, ag INTEGER, lu INTEGER,
    owner TEXT
);

CREATE TABLE IF NOT EXISTS inventory (
    slot INTEGER PRIMARY KEY,
    item_id TEXT,
    item_type TEXT,
    name TEXT,
    description TEXT,
    value INTEGER,
    extra_data TEXT
);

CREATE TABLE IF NOT EXISTS social_link (
    id TEXT PRIMARY KEY,
    rank INTEGER,
    points INTEGER
);

CREATE TABLE IF NOT EXISTS quest_progress (
    quest_id TEXT PRIMARY KEY,
    accepted INTEGER,
    completed INTEGER,
    rewarded INTEGER
);
```

## SaveRepository（存档仓库）

### 职责
- 面向 `GameManager` 提供高阶接口：
  - `saveAll(...)` / `loadAll(...)`
  - `saveCharacter` / `loadCharacter`
  - `saveInventory` / `loadInventory`
  - `savePersonas` / `loadPersonas`
  - `saveSocialLinks` / `loadSocialLinks`
  - `saveQuests` / `loadQuests`
- 将 domain 对象与 SQL schema 解耦。

### 关键约定
- 只保存基础属性；装备加成在装备时重新计算。
- Persona 以行为单位保存，技能和抗性通过 `extra_data` JSON 序列化。

## 数据库文件位置

- 开发环境：`campus-rpg/build/save.db`（与可执行文件同目录）。
- 发布时：用户数据目录（如 `%APPDATA%`）。

## 错误处理

- 打开失败时输出错误日志并返回失败状态。
- 查询失败时打印 SQL 错误信息供调试。
