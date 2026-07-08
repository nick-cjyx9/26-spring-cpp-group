# Social Link 机制层 API 外接文档

> 对应 issue #28「[P0][core+ui] Social Link 机制完善与社群面板」机制层部分。
> UI 层（SocialLinkScene / DialogueScene 增强 / 奶龙音效播放）见本文档末「UI 对接预留」一节，等 UI 图后再实现。

## 1. 设计概览

```text
DialogueScene ──talkToNpc(id)──▶ GameManager
                                  │
                                  ├─ SocialLinkManager::addPoints(id, 10)
                                  │     └─ SocialLink::addPoints → 自动 rankUp
                                  │           └─ pendingRankUps_ 记录
                                  ├─ rankUpCallback_(id, newRank)   ← UI 注册：播奶龙音效 / 弹特效
                                  ├─ currentPersona->gainExp(...)   → Persona 升 1 级
                                  ├─ currentPersona->learnSkill(...)（若该 rank 配置了新技能）
                                  └─ return dialogueFor(id)         → 当前 rank 对话文本
```

核心原则：

- **core 层零外部依赖**：`SocialLink` / `SocialLinkManager` 不 include SFML / SQLite，可被纯 C++ 单测链接。
- **奖励是 rank 的函数**：rank 决定奖励，奖励本身不持久化；`SaveRepository` 只存 `id / rank / points`（已存在，无需改动 data 层）。
- **事件解耦**：rank-up 通过 `std::function` 回调通知 UI，机制层不碰 audio / 渲染。

## 2. core 层 API

### 2.1 `SocialLink`（`src/core/SocialLink.h`）

单个 NPC 的羁绊。Rank 0..10，每 rank 拥有独立对话文本 + 可选奖励。

```cpp
class SocialLink {
public:
    static constexpr int kMaxRank = 10;

    SocialLink(std::string id, std::string name, std::string arcana);

    // identity
    const std::string &id() const;
    const std::string &name() const;
    const std::string &arcana() const;

    // progression
    int rank() const;
    int points() const;
    int pointsToNextRank() const;        // (rank+1)*20，满级返回 0
    bool canRankUp() const;
    bool isMaxRank() const;
    void rankUp();                       // 单步升 rank（扣点）
    int  addPoints(int points);          // 批量加，自动升 rank，返回本次升了几级

    // per-rank data
    void setRankData(int rank, SocialLinkRankData data);
    const SocialLinkRankData *rankData(int rank) const;
    const SocialLinkRankData *currentRankData() const;
    const SocialLinkReward   *currentReward() const;
    std::string currentDialogue() const;
};
```

`addPoints` 行为变化（相对原 stub）：**返回本次升的 rank 数**（0 = 没升 / 已满级）。旧调用 `link.addPoints(25)` 不依赖返回值，向后兼容。

### 2.2 `SocialLinkReward` / `SocialLinkRankData`

```cpp
struct SocialLinkReward {
    // 到达此 rank 时，当前 Persona 升多少级。
    // 默认 1，0 表示不升级。
    int personaLevels = 1;

    // 非空 = 当前 Persona 学会该技能。
    std::shared_ptr<Skill> newSkill;

    bool hasReward() const { return personaLevels > 0 || newSkill != nullptr; }
};

struct SocialLinkRankData {
    std::string dialogue;                // 该 rank 的对话文本
    SocialLinkReward reward;             // 到达此 rank 时解锁的奖励
};
```

### 2.3 `SocialLinkManager`（`src/core/SocialLinkManager.h`）

```cpp
class SocialLinkManager {
public:
    // CRUD
    void addLink(const SocialLink &link);
    void addLink(SocialLink &&link);
    SocialLink       *getLink(const std::string &id);
    const SocialLink *getLink(const std::string &id) const;
    std::vector<SocialLink *>       allLinks();
    std::vector<const SocialLink *> allLinks() const;
    size_t linkCount() const;

    // —— 进度 API ——
    int addPoints(const std::string &id, int points);  // 返回升的 rank 数，并记 pending
    bool hasPendingRankUps() const;
    std::vector<std::string> consumePendingRankUps();  // 取走并清空 pending

    // —— 对话 API ——
    std::string dialogueFor(const std::string &id) const;

    // —— 奖励查询 API ——
    // 取某个 link 在当前 rank 解锁的技能（如果有）。
    std::shared_ptr<Skill> currentSkillReward(const std::string &id) const;
};
```

## 3. Character / Persona 集成（coordination owner: nick-cjyx9）

Social Link 的奖励不再直接加在 `Character` 上，而是转化为 **当前 Persona 的等级成长**。

- `Character` 移除 Social Link 属性加成字段。
- `Persona` 新增等级 / 经验字段：`level_`、`exp_`、`expToNextLevel_`。
- Rank 提升时，GameManager 调用 `currentPersona->gainExp(expToNextLevel_)` 使其直接升 1 级。
- Persona 升级后基础三维按固定倍率成长（如 `baseStat *= 1.05`）。

最终属性计算（供 BattleSystem 使用）：

```text
finalStat = (baseStat + equipmentBonus) × levelMultiplier
levelMultiplier = 1 + (Persona.level - 1) × 0.05
```

## 4. GameManager 集成（`src/manager/GameManager.*`）

### 4.1 对话入口

```cpp
std::string GameManager::talkToNpc(const std::string &socialLinkId);
```

`DialogueScene::update()` 在 `firstFrame_` 时调用此方法替代直接 `link->addPoints(10)`。该方法内部：

1. 记录调用前 rank 与当前 Persona 等级；
2. `socialLinkManager_.addPoints(id, 10)`（每次对话固定 +10 点）；
3. 若 rank 提升：
   - 对每一级升 rank，调用 `currentPersona->gainExp(...)` 使当前 Persona 升 1 级；
   - 若该 rank 配置了 `newSkill`，让当前 Persona 学会该技能；
   - 对每一级升 rank 调一次 `rankUpCallback_(id, newRank)`；
4. 返回 `dialogueFor(id)` —— 即该 NPC 当前 rank 的对话文本。

### 4.2 奖励应用

```cpp
void GameManager::recomputeSocialLinkBonuses();
```

旧逻辑已废弃，保留空函数或重命名为 `applySocialLinkRewards()` 以兼容旧调用点。新逻辑在 `talkToNpc` 内部直接处理：

- 记录调用前 Persona 等级；
- 对每一级升 rank，调用 `currentPersona->gainExp(currentPersona->expToNextLevel())` 使其升 1 级；
- 若当前 rank 的 `SocialLinkReward::newSkill` 非空，调用 `currentPersona->learnSkill(...)`；
- 触发 `rankUpCallback_(id, newRank)` 供 UI 播特效。

调用时机：

- 每次 `talkToNpc` 后自动应用；
- `newGame()` / `load()` 后无需重算（Persona 等级已从存档恢复）。

### 4.3 Rank-Up 事件回调（奶龙音效接入点）

```cpp
using RankUpCallback = std::function<void(const std::string &socialLinkId, int newRank)>;
void GameManager::setRankUpCallback(RankUpCallback cb);
```

**这是机制层给 UI 层预留的音效 / 特效接入点。** 机制层本身不依赖 SFML audio，UI 层（`main.cpp` 或场景）注册回调后即可在 rank up 时播奶龙笑 / 火舞音效、弹 Rank Up 横幅。

默认 NPC 设计（3 个，满足 issue「至少 3 个 NPC」要求）：

| id | name | arcana | 奖励设计 |
| --- | --- | --- | --- |
| `sl_yosuke` | Yosuke | Magician | 每升 1 rank，当前 Persona 升 1 级；rank 3 / 6 / 9 额外学会新技能 |
| `sl_chie` | Chie | Chariot | 同上 |
| `sl_yukiko` | Yukiko | Priestess | 同上 |

每条 link 的 rank 0..10 均填有独立对话文本（见 `GameManager::initSocialLinkRankData`）。

## 5. 数据持久化（已存在，未改动）

`SaveRepository` 的 `social_link` 表（`id / rank / points`）已实现 save/load。奖励由 rank 派生（Persona 升级 + 新技能），对话/奖励定义在 `initSocialLinkRankData` 中硬编码，存档只存进度。

读档路径 `loadSocialLinks` 会按存档 rank 调 `rankUp()` 把进度推回去，再 `addPoints(points)` 恢复剩余点数。读档后需恢复当前 Persona 的等级与已学技能（由 `GameManager::loadFromSlot` 负责从默认 Persona 快照中回贴技能）。

## 6. UI 对接预留（待 UI 图后实现）

机制层已为以下 UI 需求留好接口，UI 层只需消费，无需改 core：

### 6.1 DialogueScene 增强

- 显示 NPC name / rank：`socialLinkManager().getLink(id)->name() / ->rank()`（已有）。
- 显示当前 rank 对话：`talkToNpc()` 返回值即对话文本，已存入 `DialogueScene::dialogueText_`。
- Rank Up 弹专属提示 / 特效：注册 `setRankUpCallback`，在回调里置一个 `pendingBanner_` 状态，`render()` 里画横幅。

### 6.2 奶龙音效播放（重点）

机制层提供事件，UI 层负责播放。约定如下：

- **音效文件路径**：`resources/sounds/nailong_rankup.ogg`（或 `.wav`）。
  - `resources/sounds/` 目录已创建（含 `.gitkeep`），音效文件由用户自行放入（版权素材）。
  - 备选文件名：`nailong_laugh.ogg`（奶龙笑）/ `nailong_firedance.ogg`（奶龙火舞），UI 可随机二选一增加趣味。
- **CMake**：需在主 exe 链接 `sfml-audio`（当前未链接）。`CMakeLists.txt` 的 `target_link_libraries(${PROJECT_NAME} ...)` 块加 `sfml-audio`；Windows 上 `copy_if_different` 循环里加 `sfml-audio`。
- **注册回调**（`main.cpp`，在 `GameManager::instance().newGame(...)` 之后）：

```cpp
#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>

// 全局持有，避免每次回调重新加载
static sf::SoundBuffer g_nailongBuffer;
static sf::Sound       g_nailongSound;
static bool            g_nailongLoaded = false;

GameManager::instance().setRankUpCallback(
    [](const std::string & /*socialLinkId*/, int /*newRank*/) {
        if (!g_nailongLoaded)
        {
            if (!g_nailongBuffer.loadFromFile("resources/sounds/nailong_rankup.ogg"))
                return; // 文件缺失则静默
            g_nailongSound.setBuffer(g_nailongBuffer);
            g_nailongLoaded = true;
        }
        g_nailongSound.play();
    });
```

> 注：回调在 `talkToNpc` 内同步触发，运行于主线程，可直接调 `sf::Sound::play`。

### 6.3 SocialLinkScene / CharacterScene 社群面板

读取接口（只读，无副作用）：

```cpp
for (const SocialLink *link : GameManager::instance().socialLinkManager().allLinks()) {
    // link->name()         → NPC 名
    // link->arcana()       → 阿尔卡那（用于塔罗牌图标 / 分类）
    // link->rank()         → 当前 rank (0..10)
    // link->points()       → 当前点数
    // link->pointsToNextRank() → 距下一 rank 还差多少
    // link->isMaxRank()    → 是否满级
}
```

头像 / 占位图：UI 自行从 `resources/textures/` 取（如 `npc_yosuke.png`），core 不持有纹理资源。

## 7. 单元测试覆盖（`tests/test_core.cpp`）

新增 7 项（共 50 项全绿）：

| 测试函数 | 覆盖点 |
| --- | --- |
| `testSocialLinkAddPointsReturnsRanksGained` | 返回值正确、跨多级升 rank、剩余点数 |
| `testSocialLinkMaxRankCapsAtTen` | rank 上限 10、满级后再加点不溢出 |
| `testSocialLinkRankDataDialogueAndReward` | per-rank 对话切换、reward 读取 |
| `testSocialLinkManagerAddPointsNotifiesRankUp` | pending 事件记录 / 消费 / 清空、不存在 link 不触发 |
| `testSocialLinkManagerDialogueForCurrentRank` | 对话随 rank 切换、missing link 返回空 |
| `testSocialLinkManagerSkillReward` | rank 奖励中的新技能正确授予当前 Persona |
| `testPersonaLevelsFromSocialLink` | Social Link 升级使当前 Persona 升级、基础三维成长 |

## 8. 与其他模块的边界 / 协调点

| 文件 | 改动 | owner | 状态 |
| --- | --- | --- | --- |
| `src/core/SocialLink.*` | 扩展 reward/rankData/pending | nick-cjyx9（core） | ✅ 已改 |
| `src/core/SocialLinkManager.*` | 扩展 progression/dialogue/聚合 API | nick-cjyx9 | ✅ 已改 |
| `src/core/Character.*` | 移除 sl bonus 字段；持有 ownedPersonas_ | **nick-cjyx9（coordination）** | ⏳ 待实现 |
| `src/core/Persona.*` | 新增 level/exp/三维成长 / 技能学习 | nick-cjyx9（core） | ⏳ 待实现 |
| `src/manager/GameManager.*` | talkToNpc 改奖励为 Persona 升级 + 学技能 | **nick-cjyx9（coordination）** | ⏳ 待实现 |
| `src/scenes/DialogueScene.*` | 改用 talkToNpc（最小改动） | 场景 owner | ✅ 已改（仅一行调用替换 + 一个成员） |
| `src/data/SaveRepository.*` | **未改动** | W0606 | 无需 PR |
| `CMakeLists.txt` | 链接 `sfml-audio` | UI 实现 | ⏳ UI 层再做 |
| `src/main.cpp` | 注册奶龙音效回调 | UI 实现 | ⏳ UI 层再做 |
| `resources/sounds/` | 放音效文件 | 用户 | ⏳ 目录已建，待素材 |
| `tests/test_core.cpp` | 追加 7 项单测 | tests owner | ✅ 已改，按 AGENTS.md 挂在自有测试函数下 |

## 9. 已知遗留 / 后续

- **新技能来源**：`SocialLinkReward::newSkill` 字段已预留，当前 rank 奖励直接让当前 Persona 学会该技能；后续若希望「学习某通用被动技能」，可扩展为技能池。
- **多对话选项 / 分支**：当前每 rank 一条固定对话（符合 `docs/README.md §3.2 简化设计`）；后续若加选项，可把 `SocialLinkRankData::dialogue` 升级为 `struct { std::vector<std::string> options; std::vector<int> pointValues; }`。
- **Persona 等级上限**：当前设计为无上限；若后续需要封顶，可在 `Persona::levelUp()` 中加入等级上限判断。
