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

- 游戏中有 **10 名固定 NPC**，每名 NPC 拥有 **2–3 个 Arcana**，所有 NPC 合起来覆盖 22 张大阿卡纳。
- 每天早晨从 NPC 池中**随机抽取 4 名**（城镇 2 名 + 学校 2 名）出现在固定位置。
- 玩家靠近并按交互键，触发简短多行对话。
- 每名 NPC 每天最多可对话 2 次；对话后该 NPC 的 Social Link 进度增加。
- 进度满后 Rank 提升，解锁奖励：
  - **所有已拥有且 Arcana 与该 NPC 任一 Arcana 匹配的 Persona 立即升 1 级**（基础三维按固定倍率成长）；若尚未拥有匹配 Persona，则回退到当前 Persona 升级。
  - 特定 Rank 可能让**当前 Persona 学会新技能**。

**简化设计**：

- 每个 Rank 对应一段固定多行对话。
- Rank 提升时切换为新对话，表示关系推进。
- 不做复杂分支、约会、冲突。
- Persona 等级暂时无上限。

### 3.3 Persona 机制

- 主角携带一个**当前 Persona**，并持有一个**已拥有 Persona 列表**（战斗中可切换）。
- 游戏开始时，主角固定获得 **Orpheus（Fool）** 作为初始当前 Persona（平衡三维，初始携带火属性攻击 + 恢复技能）。
  - 其余 Persona 通过战斗掉落、商店购买获得。
- Persona 提供：
  - **三维属性**：力量（Strength）、魔力（Magic）、速度（Speed）。
  - 元素抗性（Weak / Resist / Null / Drain / Repel）。
  - 可使用的技能。
- 获得方式：
  - **击败敌人直接掉落 Persona**（战斗结束结算界面展示）。
  - **商店直接购买 Persona**（购买后加入已拥有列表）。
- Persona 升级唯一来源：Social Link Rank 提升。
- 战斗中可切换 Persona（消耗一回合）。

**属性计算公式**：

```text
最终属性 = (基础属性 + 装备加成) × 等级倍率
等级倍率 = 1 + (Persona 等级 - 1) × 0.05
```

- 基础属性已包含 Social Link 升级带来的成长。
- 每次 Social Link Rank 提升时，当前 Persona 升 1 级，基础三维按固定倍率（如 ×1.05）成长。
- 装备加成按槽位不同而有偏向：
  - Weapon：偏向力量
  - Armor：偏向魔力（防御类技能效果按魔力计算）
  - Accessory：偏向速度
  - Relic：混合加成

**合成（可选扩展）**：

- 两个 Persona 合成新 Persona。
- 新 Persona 的阿尔卡那和等级由父母决定，继承 1~2 个技能。
- 首期实现可跳过，作为后续扩展。

### 3.4 简化回合制战斗

#### 参战单位

玩家 + 1~N 名敌人。玩家与敌人都只有**三维属性**：力量 / 魔力 / 速度，没有独立防御值，以 HP 作为生存缓冲。

#### 先攻与行动顺序

- 战斗开始时，所有参战单位按速度 roll 一次 initiative：
  ```text
  initiative = speed × (1 + random(-10%, +10%))
  ```
- 按 initiative 从高到低排成固定顺序，整场战斗按此顺序循环行动。
- 速度相同时，玩家优先于敌人。

#### 回合内可执行的行动

玩家回合内可选择：

- **物理攻击**：造成物理属性伤害。
- **技能**：消耗 SP，带元素属性。
- **切换 Persona**：更换当前 Persona，**消耗本回合**。
- **逃跑**：尝试脱离战斗。
- **道具**：**免费动作**，在自己的回合内可随时使用（每回合最多一次，避免无限回复），不结束回合。
  - 目前道具仅包括恢复 HP / SP 的消耗品。

#### 伤害公式

- **物理攻击伤害**：
  ```text
  damage = basePhysicalPower × (1 + attacker.strength / 10) × (1 ± 5%)
  ```
  可被目标闪避。
- **技能伤害**：
  ```text
  damage = skillPower × (1 + attacker.magic / 10) × affinityMultiplier × (1 ± 5%)
  ```
  技能不会被闪避（命中由设计决定）。
- **Affinity 倍率**（保留但无 1 More）：
  - Weak：1.5 倍伤害
  - Resist：0.5 倍伤害
  - Null / Drain / Repel：正常生效（0 伤害 / 吸血 / 反弹）

#### 闪避

- 仅**物理攻击**可能被闪避。
- 闪避率基于速度差：
  ```text
  dodgeRate = clamp((defender.speed - attacker.speed) / attacker.speed × 0.5, 0.05, 0.50)
  ```
  即速度相等时基础闪避率 5%，defender 速度达到 attacker 两倍时达到上限 50%。
- 敌人同样适用闪避。

#### 敌人 AI

- 每个敌人实例拥有一套**固定的出招循环**（技能 ID 序列 + 普通攻击占位）。
- 按循环顺序每回合执行下一个动作，循环往复。
- 敌人随玩家等级成长，属性按公式缩放：
  ```text
  enemyStat = baseStat × (1 + (playerLevel - 1) × 0.1)
  ```

#### 战斗流程

1. 进入战斗时生成 initiative 队列。
2. 按队列顺序，每个存活单位行动一次。
3. 玩家选择行动；敌人按固定循环行动。
4. 重复直到一方全灭、逃跑成功或玩家战败被救回。
5. 战斗结束后弹出结算界面：胜利显示金币、EXP、掉落 Persona；失败显示被救回并恢复 HP/SP。

#### SP 恢复

- 进入战斗时 SP 默认为满。
- 战斗中仅能通过道具恢复 SP。
- 战斗结束（回到城镇 / 夜晚地图）后 SP 回满。

### 3.5 商店与背包

**商店出售**：

- 恢复 HP 道具（面包、便当、能量饮料等）
- 恢复 SP 道具（咖啡、提神药等）
- 装备（按槽位：Weapon / Armor / Accessory / Relic），装备加成直接作用于当前 Persona 的三维
- Persona（直接购买后加入已拥有列表；出售范围暂时与战斗掉落范围一致）

**背包存放**：

- 上述所有购买的道具
- 战斗掉落

### 3.6 任务系统

`Quest` / `QuestManager`：

- 按 `U` 打开任务日志：查看、接受、完成、领奖。
- 当前默认任务均为击杀类（`kill:N`）；击败阴影后自动累计击杀进度，达到目标即可在任务面板领奖。
- 代码层仍保留 `collect:id:N` 扩展接口，但默认任务不再依赖背包中不存在的收集物。

### 3.7 等级成长

#### 角色等级

`Character::gainExp` / `levelUp`：

- 角色经验来源：
  - 完成任务：约 90%
  - 击败敌人：约 10%
- 角色升级提升 **HP / SP 上限**。
- 角色等级决定敌人的缩放等级。

#### Persona 等级

- Persona **只能通过 Social Link Rank 提升**获得成长。
- 每次 Social Link Rank 提升，**所有匹配该 NPC Arcana 的已拥有 Persona 各升 1 级**（未拥有匹配 Persona 时回退到当前 Persona）。
- Persona 升级后：
  - 基础三维按固定倍率成长（如 ×1.05）。
  - 特定 Rank 可能学会新技能。
- Persona 等级暂时无上限。

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
| --- | --- |
| **Engine** | 窗口、事件循环、输入、2D 渲染。通过抽象接口 `IRenderer`/`IWindow`/`IInput` 与上层解耦。 |
| **Manager** | `GameManager` 单例，持有全局状态，协调场景切换、保存/加载。 |
| **Core** | 纯 C++ 领域模型，无外部依赖，供单元测试直接链接。 |
| **Data** | SQLite3 持久化，`DatabaseManager` + `SaveRepository`。 |

### CMake 库划分

| 库 | 源码 | 依赖 |
| --- | --- | --- |
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
    strength INTEGER, magic INTEGER, speed INTEGER,
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

## 8. 课程要求覆盖

| 要求 | 说明 |
| --- | --- |
| 六点基础功能 | 角色/背包/商店/任务/战斗/成长全部覆盖 |
| OOP 四项特征 | 封装、继承、多态、关联均体现 |
| 至少六个类 | Character/Item/Quest/Enemy/Shop/GameManager |
| 至少两种 STL | vector、map 大量使用 |
| 数据持久化 | SQLite3 |
| 人机交互 | SFML 图形界面 + 清晰场景流程 |
| 挑战任务 | 数据库、图形界面、STL 高级应用、软件工程实践 |
