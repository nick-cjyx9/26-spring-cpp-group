# Campus RPG 2D 简化版设计文档（最终版）

## 1. 项目定位

基于《女神异闻录 4》（Persona 4）灵感的 2D 校园 RPG，但大幅简化：
- 只做**早晨城镇/学校 Social Link** + **夜晚同地图战斗**。
- 战斗为**简化回合制**，不做 1 More / 总攻击 / 倒地机制。
- 保留 **Persona 装备/切换**，合成作为可选扩展。
- 保留 **商店、背包、任务、等级成长** 等基础 RPG 系统。

技术栈：C++20 + SFML 2.6 + SQLite3 + CMake。

---

## 2. 最终玩法循环

```text
新的一天（早晨）
    ↓
城镇/学校 2D 地图，玩家自由移动
    ↓
可选活动：
  ├─ 与 NPC 对话 → 简单对话 + Social Link 进度
  ├─ 进入商店 → 购买/出售道具
  ├─ 打开背包/角色面板 → 使用道具、装备、切换 Persona
  ├─ 查看任务
  └─ 选择“等到夜晚”
    ↓
夜晚降临，地图色调变暗，出现阴影（Shadow）
    ↓
玩家碰到阴影 → 进入回合制战斗
    ↓
战斗胜利：获得 EXP、金币、掉落道具/Persona
    ↓
回家睡觉 → 进入下一天
```

---

## 3. 核心系统

### 3.1 地图与移动

- 顶视角（Top-down）2D 地图。
- 玩家用 **WASD / 方向键** 移动。
- 地图元素：`Wall`（不可通过）、`Door`（传送/切换区域）、`NpcEntity`（对话）、`EnemyEntity`（夜晚出现）。
- 碰撞检测：AABB 矩形碰撞。

### 3.2 Social Link（早晨）

- 每天早晨，特定 NPC 出现在固定位置。
- 玩家靠近并按交互键，触发简短对话。
- 对话后该 NPC 的 Social Link 进度增加。
- 进度满后 Rank 提升，解锁奖励：
  - 给当前 Persona 属性加成
  - 或给角色额外技能

**简化设计**：
- 每段 Social Link 只有一段固定对话。
- Rank 提升时切换为新对话，表示关系推进。
- 不做复杂分支、约会、冲突。

### 3.3 Persona 机制

- 主角携带一个当前 Persona。
- Persona 提供：
  - 五维属性（力/魔/耐/速/运）
  - 元素抗性（Weak/Resist/Null/Drain/Repel）
  - 可使用的技能
- 战斗中可切换 Persona（消耗一回合）。
- 获得方式：击败敌人掉落、商店购买。

**合成（可选扩展）**：
- 两个 Persona 合成新 Persona。
- 新 Persona 的阿尔卡那和等级由父母决定，继承 1~2 个技能。
- 首期实现可跳过，作为后续扩展。

### 3.4 简化回合制战斗

战斗指令：
- 攻击（物理）
- 技能（消耗 SP，带属性）
- 道具
- 防御
- 切换 Persona
- 逃跑

战斗流程：
1. 玩家选择一次行动。
2. 行动结算。
3. 若敌人存活，敌人行动一次。
4. 循环直到一方倒下或逃跑。

**抗性保留但无 1 More**：
- Weak：1.5 倍伤害
- Resist：0.5 倍伤害
- Null/Drain/Repel：正常生效

### 3.5 商店与背包

**商店出售**：
- 恢复 HP 道具（面包、便当、能量饮料）
- 恢复 SP 道具（咖啡、提神药）
- 装备（训练剑、校服外套、运动鞋）
- Persona 契约书（获得特定 Persona）

**背包存放**：
- 上述所有购买的道具
- 战斗掉落

### 3.6 任务系统

保留现有 `Quest` / `QuestManager`：
- 查看、接受、完成、领奖。
- 条件支持 `kill:N`，可扩展 `collect:id:N`、`level:N`。

### 3.7 等级成长

`Character::gainExp` / `levelUp`：
- 升级提升 HP/SP 上限、五维、攻防。
- 升级时弹出成长面板展示属性增长。

---

## 4. 架构设计

```text
┌─────────────────────────────────────┐
│ Engine / Rendering Layer            │
│  IRenderer / IWindow / IInput       │
│  SFML 实现 / Mock 实现               │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│ Manager Layer                       │
│  GameManager（单例总控）              │
│  场景切换、存档/读档入口               │
└──────────────┬──────────────────────┘
               │
    ┌──────────┴──────────┐
    ▼                     ▼
┌──────────────┐   ┌──────────────┐
│ Core         │   │ Data         │
│ 纯 C++ 领域模型 │   │ SQLite3 持久化 │
└──────────────┘   └──────────────┘
```

### 层职责

| 层 | 说明 |
|---|---|
| **Engine** | 窗口、事件循环、输入、2D 渲染。通过抽象接口 `IRenderer`/`IWindow`/`IInput` 与上层解耦。 |
| **Manager** | `GameManager` 单例，持有全局状态，协调场景切换、保存/加载。 |
| **Core** | 纯 C++ 领域模型，无外部依赖，供单元测试直接链接。 |
| **Data** | SQLite3 持久化，`DatabaseManager` + `SaveRepository`。 |

### CMake 库划分

| 库 | 源码 | 依赖 |
|---|---|---|
| `CampusRPGCore` | `src/core/` | 纯 C++ |
| `CampusRPGAppLib` | `src/data/` + `src/manager/` + `src/engine/interfaces/` | `CampusRPGCore` + SQLite3 |
| `CampusRPG` | `src/engine/sfml/` + `src/main.cpp` | `CampusRPGAppLib` + SFML |
| `CampusRPGTests` | `tests/test_core.cpp` + `tests/mocks/` | `CampusRPGCore` + Mock 实现 |

---

## 5. 抽象基类与 Mock

### 引擎抽象接口

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void clear() = 0;
    virtual void present() = 0;
    virtual void drawRect(const Rect& rect, Color color) = 0;
    virtual void drawTexture(const std::string& textureId, const Vec2& pos) = 0;
    virtual void drawText(const std::string& text, const Vec2& pos, int size) = 0;
};

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    virtual void pollEvents(IInput& input) = 0;
    virtual IRenderer& renderer() = 0;
};

class IInput {
public:
    virtual ~IInput() = default;
    virtual bool isKeyPressed(Key key) const = 0;
    virtual bool wasKeyJustPressed(Key key) = 0;
};
```

实现：
- `SfmlRenderer` / `SfmlWindow` / `SfmlInput`
- `MockRenderer` / `MockWindow` / `MockInput`（测试用）

### 核心抽象基类

- `Item` → `FoodItem` / `PotionItem` / `SpItem` / `EquipmentItem` / `PersonaItem`
- `Enemy` → `Slime` / `Goblin` / `Boss`
- `Entity` → `PlayerEntity` / `NpcEntity` / `EnemyEntity`
- `Scene` → `TownScene` / `BattleScene` / `ShopScene` / `DialogueScene`

---

## 6. 数据持久化

SQLite 表：

```sql
-- 角色基础属性
CREATE TABLE character (
    id INTEGER PRIMARY KEY CHECK(id = 1),
    name TEXT, level INTEGER, hp INTEGER, max_hp INTEGER,
    sp INTEGER, max_sp INTEGER, exp INTEGER, exp_to_next INTEGER,
    gold INTEGER, pos_x REAL, pos_y REAL, is_night INTEGER,
    current_persona_id TEXT
);

-- Persona
CREATE TABLE persona (
    id TEXT PRIMARY KEY,
    name TEXT, arcana TEXT, level INTEGER,
    st INTEGER, ma INTEGER, en INTEGER, ag INTEGER, lu INTEGER,
    owner TEXT  -- 'player' 或 NULL
);

-- 背包
CREATE TABLE inventory (
    slot INTEGER PRIMARY KEY,
    item_id TEXT, item_type TEXT, name TEXT, description TEXT,
    value INTEGER, extra_data TEXT
);

-- Social Link
CREATE TABLE social_link (
    id TEXT PRIMARY KEY,
    rank INTEGER, points INTEGER
);

-- 任务进度
CREATE TABLE quest_progress (
    quest_id TEXT PRIMARY KEY,
    accepted INTEGER, completed INTEGER, rewarded INTEGER
);
```

---

## 7. 实现路线

### 第一阶段：架构与 2D 引擎
- [ ] 创建 `IRenderer`/`IWindow`/`IInput` 抽象接口
- [ ] SFML 具体实现
- [ ] Mock 实现（供测试）
- [ ] 游戏循环 `GameLoop`

### 第二阶段：Core 层重写
- [ ] 保留并改造 `Character`/`Inventory`/`Item`/`Enemy`/`Shop`/`Quest`
- [ ] 新增 `Persona`/`SocialLink`/`Entity`/`TileMap`
- [ ] 简化 `BattleSystem`

### 第三阶段：Manager 与 Data
- [ ] `GameManager` 接入新 Core 和场景
- [ ] SQLite schema 扩展
- [ ] `SaveRepository` 支持新字段

### 第四阶段：场景与 UI
- [ ] `TownScene`：早晨移动 + NPC 对话
- [ ] `NightScene`：同地图 + 敌人遭遇
- [ ] `BattleScene`：简化回合制
- [ ] `ShopScene` / `InventoryScene` / `CharacterScene`

### 第五阶段：测试与文档
- [ ] Core 层单元测试
- [ ] Mock 场景测试
- [ ] 更新 UML 图与报告

---

## 8. 课程要求覆盖

| 要求 | 说明 |
|---|---|
| 六点基础功能 | 角色/背包/商店/任务/战斗/成长全部覆盖 |
| OOP 四项特征 | 封装、继承、多态、关联均体现 |
| 至少六个类 | Character/Item/Quest/Enemy/Shop/GameManager |
| 至少两种 STL | vector、map 大量使用 |
| 数据持久化 | SQLite3 |
| 人机交互 | SFML 图形界面 + 清晰场景流程 |
| 挑战任务 | 数据库、图形界面、STL 高级应用、软件工程实践 |
