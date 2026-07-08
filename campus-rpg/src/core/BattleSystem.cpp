#include "BattleSystem.h"

#include "Item.h"

#include <algorithm>
#include <random>

namespace
{
    thread_local std::mt19937 rng{std::random_device{}()};

    int randomVariance(int base)
    {
        std::uniform_int_distribution<int> dist(-base / 20, base / 20);
        return std::max(1, base + dist(rng));
    }
} // namespace

void BattleSystem::startBattle(Character &player, Enemy &enemy)
{
    auto enemyCopy = enemy.clone();
    std::vector<std::unique_ptr<Enemy>> vec;
    vec.push_back(std::move(enemyCopy));
    startBattle(player, std::move(vec));
}

void BattleSystem::startBattle(Character &player, std::vector<std::unique_ptr<Enemy>> enemies)
{
    player_ = &player;
    player.recoverSp(player.maxSp());
    enemies_ = std::move(enemies);
    log_.clear();
    turnOrder_.clear();
    currentTurnIndex_ = 0;
    enemyTurnCounter_ = 0;
    fled_ = false;
    droppedPersonaIds_.clear();

    appendLog("Battle started: " + player.name() + " vs shadows");
    for (const auto &enemy : enemies_)
    {
        if (enemy)
            appendLog(enemy->battleCry());
    }

    buildTurnOrder();

    // If the first turn belongs to an enemy, run enemy turns immediately.
    if (!isPlayerTurn() && !isOver())
        processEnemyTurns();
}

void BattleSystem::buildTurnOrder()
{
    turnOrder_.clear();

    // Player entry.
    int playerSpeed = player_ ? player_->speed() : 0;
    std::uniform_real_distribution<double> dist(-0.1, 0.1);
    TurnEntry playerEntry;
    playerEntry.isPlayer = true;
    playerEntry.enemyIndex = 0;
    playerEntry.initiative = static_cast<int>(playerSpeed * (1.0 + dist(rng)));
    turnOrder_.push_back(playerEntry);

    // Enemy entries.
    for (size_t i = 0; i < enemies_.size(); ++i)
    {
        if (!enemies_[i] || !enemies_[i]->isAlive())
            continue;
        TurnEntry entry;
        entry.isPlayer = false;
        entry.enemyIndex = i;
        entry.initiative = static_cast<int>(enemies_[i]->speed() * (1.0 + dist(rng)));
        turnOrder_.push_back(entry);
    }

    // Sort by initiative descending; player wins ties.
    std::sort(turnOrder_.begin(), turnOrder_.end(),
              [](const TurnEntry &a, const TurnEntry &b)
              {
                  if (a.initiative != b.initiative)
                      return a.initiative > b.initiative;
                  return a.isPlayer && !b.isPlayer;
              });
}

bool BattleSystem::isPlayerTurn() const
{
    if (turnOrder_.empty() || isOver())
        return false;
    return turnOrder_[currentTurnIndex_].isPlayer;
}

void BattleSystem::advanceTurn()
{
    if (turnOrder_.empty())
        return;
    currentTurnIndex_ = (currentTurnIndex_ + 1) % turnOrder_.size();
}

bool BattleSystem::playerAttack(size_t enemyIndex)
{
    if (isOver() || !isPlayerTurn())
        return false;

    Enemy *target = enemyAt(enemyIndex);
    if (!target || !target->isAlive())
        return false;

    if (rollDodge(player_->speed(), target->speed()))
    {
        appendLog(target->name() + " dodged " + player_->name() + "'s attack!");
    }
    else
    {
        int damage = physicalDamage(player_->attack());
        target->takeDamage(damage);
        appendLog(player_->name() + " attacks " + target->name() + " for " +
                  std::to_string(damage) + " damage.");
    }

    advanceTurn();
    processEnemyTurns();
    return true;
}

bool BattleSystem::playerUseSkill(size_t enemyIndex, const Skill &skill)
{
    if (isOver() || !isPlayerTurn())
        return false;
    if (!skill.canUse(*player_))
        return false;

    Enemy *target = enemyAt(enemyIndex);
    if (!target || !target->isAlive())
        return false;

    if (skill.isHealing())
    {
        skill.use(*player_, *target);
        appendLog(player_->name() + " uses " + skill.name() + " and recovers " +
                  std::to_string(skill.power()) + " HP.");
    }
    else
    {
        skill.applyCost(*player_);
        int raw = skill.calculateRawDamage(*player_, *target);
        int damage = applyAffinityMultiplier(raw, target->affinity(skill.element()));
        target->takeDamage(damage);
        appendLog(player_->name() + " casts " + skill.name() + " on " + target->name() +
                  " for " + std::to_string(damage) + " damage.");
    }

    advanceTurn();
    processEnemyTurns();
    return true;
}

bool BattleSystem::playerUseItem(std::shared_ptr<Item> item)
{
    if (isOver() || !isPlayerTurn() || !item)
        return false;

    item->use(*player_);
    appendLog(player_->name() + " uses " + item->name() + ".");
    // Free action: do not advance turn or trigger enemy turns.
    return true;
}

bool BattleSystem::playerSwitchPersona(std::shared_ptr<Persona> persona)
{
    if (isOver() || !isPlayerTurn() || !persona)
        return false;

    player_->setPersona(persona);
    appendLog(player_->name() + " switches to " + persona->name() + ".");

    advanceTurn();
    processEnemyTurns();
    return true;
}

bool BattleSystem::playerFlee()
{
    if (isOver() || !isPlayerTurn())
        return false;

    int enemySpeed = 0;
    for (const auto &enemy : enemies_)
    {
        if (enemy && enemy->isAlive())
            enemySpeed = std::max(enemySpeed, enemy->speed());
    }

    if (rollFleeSuccess(player_->speed(), enemySpeed))
    {
        fled_ = true;
        appendLog("You fled from battle.");
        return true;
    }

    appendLog("Failed to flee!");
    advanceTurn();
    processEnemyTurns();
    return false;
}

void BattleSystem::processEnemyTurns()
{
    while (!isOver() && !isPlayerTurn())
    {
        size_t enemyIndex = turnOrder_[currentTurnIndex_].enemyIndex;
        executeEnemyTurn(enemyIndex);
        advanceTurn();
    }
}

void BattleSystem::executeEnemyTurn(size_t enemyIndex)
{
    if (enemyIndex >= enemies_.size() || !enemies_[enemyIndex] || !enemies_[enemyIndex]->isAlive())
        return;

    Enemy &enemy = *enemies_[enemyIndex];
    auto skill = enemy.chooseSkill(enemyTurnCounter_);
    ++enemyTurnCounter_;

    if (skill && !skill->isHealing())
    {
        // Enemy skill damage: power scaled by enemy magic.
        int raw = static_cast<int>(skill->power() * (1.0 + enemy.magic() / 10.0));
        raw = randomVariance(raw);
        int actual = applyAffinityMultiplier(raw, player_->affinity(skill->element()));
        player_->takeDamage(actual);
        appendLog(enemy.name() + " casts " + skill->name() + " for " +
                  std::to_string(actual) + " damage.");
    }
    else
    {
        if (rollDodge(enemy.speed(), player_->speed()))
        {
            appendLog(player_->name() + " dodged " + enemy.name() + "'s attack!");
        }
        else
        {
            int actual = physicalDamage(enemy.strength());
            player_->takeDamage(actual);
            appendLog(enemy.name() + " attacks " + player_->name() + " for " +
                      std::to_string(actual) + " damage.");
        }
    }
}

bool BattleSystem::isOver() const
{
    return fled_ || !player_->isAlive() ||
           std::none_of(enemies_.begin(), enemies_.end(),
                        [](const auto &e)
                        { return e && e->isAlive(); });
}

bool BattleSystem::playerWon() const
{
    return player_->isAlive() &&
           std::none_of(enemies_.begin(), enemies_.end(),
                        [](const auto &e)
                        { return e && e->isAlive(); });
}

bool BattleSystem::playerLost() const
{
    return !player_->isAlive();
}

Enemy *BattleSystem::enemyAt(size_t index) const
{
    if (index >= enemies_.size())
        return nullptr;
    return enemies_[index].get();
}

int BattleSystem::applyAffinityMultiplier(int rawDamage, Affinity affinity) const
{
    switch (affinity)
    {
    case Affinity::Weak:
        return static_cast<int>(rawDamage * 1.5);
    case Affinity::Resist:
        return std::max(1, rawDamage / 2);
    case Affinity::Null:
    case Affinity::Drain:
    case Affinity::Repel:
        return 0;
    default:
        return rawDamage;
    }
}

int BattleSystem::physicalDamage(int attackerStrength) const
{
    int basePower = 8;
    int raw = static_cast<int>(basePower * (1.0 + attackerStrength / 10.0));
    return std::max(1, randomVariance(raw));
}

bool BattleSystem::rollDodge(int attackerSpeed, int defenderSpeed) const
{
    double rate = (static_cast<double>(defenderSpeed) - attackerSpeed) / attackerSpeed * 0.5;
    int chance = static_cast<int>(rate * 100.0);
    chance = std::clamp(chance, 5, 50);
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng) <= chance;
}

bool BattleSystem::rollFleeSuccess(int playerSpeed, int enemySpeed) const
{
    int chance = 50 + (playerSpeed - enemySpeed) * 2;
    chance = std::clamp(chance, 10, 90);
    std::uniform_int_distribution<int> dist(1, 100);
    return dist(rng) <= chance;
}

void BattleSystem::recoverPlayerSp()
{
    if (player_)
        player_->recoverSp(player_->maxSp());
}

void BattleSystem::appendLog(const std::string &message)
{
    log_.push_back(message);
}
