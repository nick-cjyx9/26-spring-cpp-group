# 核心域模型（Core）

`src/core/` 目录下所有类均为纯 C++，不依赖 SFML/SQLite，便于单元测试。

## Character（角色）

### 职责

维护玩家属性与当前 Persona。

### 字段

```cpp
std::string name_;
int level_, hp_, maxHp_, sp_, maxSp_;
int exp_, expToNextLevel_, gold_;
int baseSt_, baseMa_, baseEn_, baseAg_, baseLu_;
int equipmentAttackBonus_, equipmentDefenseBonus_, equipmentSpeedBonus_;
std::shared_ptr<Persona> persona_;
```

### 关键行为

- `takeDamage(int damage)`：实际伤害 = `max(1, damage - defense())`。
- `heal(int amount)`：治疗至 `maxHp_` 上限。
- `consumeSp(int amount)` / `recoverSp(int amount)`：SP 管理。
- `gainExp(int amount)`：累计经验，达到阈值后升级。
- `levelUp()`：提升 HP/SP 上限、五维、攻防。
- `attack()` / `defense()` / `speed()` / `magic()`：由角色基础 + 当前 Persona + 装备加成计算。
- `setPersona(std::shared_ptr<Persona>)`：切换当前 Persona。

## Persona（人格面具）

### 职责

提供角色的战斗属性、抗性、技能。

### 字段

```cpp
std::string id_, name_, arcana_;
int level_;
int st_, ma_, en_, ag_, lu_;
std::map<Element, Affinity> affinities_;
std::vector<std::shared_ptr<Skill>> skills_;
```

### 关键行为

- `affinity(Element e)`：查询元素抗性。
- `learnSkill(std::shared_ptr<Skill>)`：学习技能。
- `findSkill(const std::string& id)`：按 ID 查找技能。

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
| `EquipmentItem` | 提升属性 | `attackBonus_`, `defenseBonus_`, `speedBonus_` |
| `PersonaItem` | 获得 Persona | `personaId_` |

## Inventory（背包）

持有 `std::vector<std::unique_ptr<Item>>`，提供添加、使用、移除、查询接口。

可设置容量上限。

## Enemy 体系

### 抽象基类 `Enemy`

```cpp
class Enemy {
public:
    virtual ~Enemy() = default;
    virtual std::string battleCry() const = 0;
    virtual std::unique_ptr<Enemy> clone() const = 0;
    // stats, affinity, skills...
};
```

### 派生类

| 类 | 特点 |
| --- | --- |
| `Slime` | 低攻低防，弱火 |
| `Goblin` | 均衡，耐物理 |
| `Boss` | 高血量，多技能 |

## BattleSystem（战斗系统）

简化回合制：

1. 玩家行动：攻击 / 技能 / 道具 / 防御 / 切换 Persona / 逃跑。
2. 敌人行动：随机选择技能或普攻。
3. 循环至一方 HP <= 0 或逃跑成功。

保留抗性影响伤害，但不做 1 More / 总攻击 / 倒地。

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
