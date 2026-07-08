# 核心域模型（Core）

`src/core/` 目录下所有类均为纯 C++，不依赖 SFML/SQLite，便于单元测试。

## Character（角色）

### 职责

维护玩家等级、HP/SP、金币与当前 Persona。战斗三维完全来自当前 Persona，角色自身不直接持有攻击/防御数值。

### 字段

```cpp
std::string name_;
int level_, hp_, maxHp_, sp_, maxSp_;
int exp_, expToNextLevel_, gold_;
std::shared_ptr<Persona> currentPersona_;
std::vector<std::shared_ptr<Persona>> ownedPersonas_;
```

### 关键行为

- `takeDamage(int damage)`：直接扣减 HP（无独立防御值，以 HP 为生存缓冲）。
- `heal(int amount)`：治疗至 `maxHp_` 上限。
- `consumeSp(int amount)` / `recoverSp(int amount)`：SP 管理。
- `gainExp(int amount)`：累计经验，达到阈值后升级；经验来源 90% 任务、10% 杀怪。
- `levelUp()`：提升 HP/SP 上限。
- `setPersona(std::shared_ptr<Persona>)`：切换当前 Persona。
- 持有 `ownedPersonas_` 列表，战斗中可切换已拥有的 Persona。

## Persona（人格面具）

### 职责

提供角色的战斗三维、元素抗性、技能。Persona 等级只能通过 Social Link Rank 提升获得成长。

### 字段

```cpp
std::string id_, name_, arcana_;
int level_;
int exp_, expToNextLevel_;
int strength_, magic_, speed_;
std::map<Element, Affinity> affinities_;
std::vector<std::shared_ptr<Skill>> skills_;
```

### 属性计算

```text
finalStat = (baseStat + equipmentBonus) × levelMultiplier
levelMultiplier = 1 + (level_ - 1) × 0.05
```

- `baseStat` 已包含 Social Link 升级带来的成长。
- 升级时基础三维按固定倍率成长（如 `baseStat *= 1.05`）。

### 关键行为

- `affinity(Element e)`：查询元素抗性。
- `learnSkill(std::shared_ptr<Skill>)`：学习技能。
- `findSkill(const std::string& id)`：按 ID 查找技能。
- `gainExp(int amount)` / `levelUp()`：升级成长。

## Skill（技能）

### 字段

```cpp
std::string id_, name_;
Element element_;
int power_, cost_;
SkillCostType costType_;  // HP or SP
bool healing_;
```

### 关键行为

- `canUse(const Character&)`：判断 SP/HP 是否足够。
- `applyCost(Character&)`：消耗 SP/HP。
- `calculateRawDamage(const Character&, const Enemy&)`：计算原始伤害。
  - 伤害技能：`skillPower × (1 + caster.magic / 10)`。
  - 恢复技能：按 `power_` 恢复 HP。
- `use(Character&, Enemy&)`：完整施放流程。

## Element / Affinity

- `Element`：Physical, Fire, Ice, Electric, Wind, Light, Dark, Almighty。
- `Affinity`：Weak, Normal, Resist, Null, Drain, Repel。

## Item 体系

### 抽象基类 `Item`

```cpp
class Item {
public:
    virtual ~Item() = default;
    virtual std::unique_ptr<Item> clone() const = 0;
    virtual void use(Character&) = 0;
    // getters...
};
```

### 派生类

| 类 | 作用 | 字段 |
| --- | --- | --- |
| `FoodItem` | 回复 HP | `healAmount_` |
| `PotionItem` | 大量回复 HP | `healAmount_` |
| `SpItem` | 回复 SP | `spAmount_` |
| `EquipmentItem` | 提升当前 Persona 三维 | `strengthBonus_`, `magicBonus_`, `speedBonus_`, `slot_` |
| `PersonaItem` | 获得 Persona（商店/掉落表示） | `personaId_` |

## Inventory（背包）

持有 `std::vector<std::unique_ptr<Item>>`，提供添加、使用、移除、查询接口。

可设置容量上限。

## Enemy 体系

### 抽象基类 `Enemy`

敌人同样只有三维：力量 / 魔力 / 速度，无独立防御值。

```cpp
class Enemy {
public:
    virtual ~Enemy() = default;
    virtual std::string battleCry() const = 0;
    virtual std::unique_ptr<Enemy> clone() const = 0;
    // strength_, magic_, speed_, affinity, skills, attackPattern_
};
```

### 派生类

| 类 | 特点 |
| --- | --- |
| `Slime` | 低力量低速度，弱元素 |
| `Goblin` | 较均衡，耐物理 |
| `Boss` | 高血量，固定多技能循环 |

### 等级缩放

敌人属性随玩家等级线性缩放：

```text
enemyStat = baseStat × (1 + (playerLevel - 1) × 0.1)
```

### 固定出招循环

每个敌人实例维护一个动作序列（`std::vector<std::string>`，元素为技能 ID 或 `"normal"`），每回合按顺序执行下一项并循环。

## BattleSystem（战斗系统）

### 行动顺序

1. 战斗开始时，所有参战单位 roll initiative：
   ```text
   initiative = speed × (1 + random(-10%, +10%))
   ```
2. 按 initiative 排序生成固定行动队列，整局按队列循环。
3. 轮到玩家时选择行动；轮到敌人时执行其固定循环中的下一动作。

### 玩家行动

- **物理攻击**：`basePhysicalPower × (1 + strength / 10)`，可被闪避。
- **技能**：消耗 SP，伤害按 `magic` 放大。
- **切换 Persona**：消耗本回合。
- **道具**：免费动作，每回合限一次，不结束回合。
- **逃跑**：基于速度差判定。

### 闪避

仅物理攻击可被闪避：

```text
dodgeRate = clamp((defender.speed - attacker.speed) / attacker.speed × 0.5, 0.05, 0.50)
```

### 战斗结束

- 一方全灭或逃跑成功时结束。
- 胜利后角色获得经验 / 金币，敌人可能直接掉落 Persona。
- 战斗结束后 SP 回满。

## Shop（商店）

- `buy(size_t index, Character&, Inventory&)`
- `sell(size_t inventoryIndex, Character&, Inventory&)`

## SocialLink（社群）

```cpp
class SocialLink {
public:
    SocialLink(std::string id, std::string name, std::string arcana);
    int rank() const;
    void addPoints(int points);
    bool canRankUp() const;
    void rankUp();
private:
    std::string id_, name_, arcana_;
    int rank_ = 0;
    int points_ = 0;
};
```

`SocialLinkManager` 用 `std::map<std::string, SocialLink>` 管理所有 NPC 羁绊。

### Rank 奖励

- Rank 提升时，**当前 Persona 升 1 级**。
- 特定 Rank 可能让**当前 Persona 学会新技能**。
- Persona 等级暂时无上限。

## TileMap / Entity

- `TileMap`：瓦片地图，包含不可通过的墙、可交互对象位置。
- `Entity`：地图上实体的抽象基类。
  - `PlayerEntity`
  - `NpcEntity`（带 Social Link）
  - `EnemyEntity`（夜晚出现，碰撞触发战斗）

## Quest / QuestManager

保留现有设计：

- `Quest`：id、name、description、condition、reward。
- `QuestManager`：accept / complete / reward。
- 条件支持 `kill:N`，可扩展 `collect:id:N`、`level:N`。
