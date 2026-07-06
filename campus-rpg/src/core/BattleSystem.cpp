#include "BattleSystem.h"

#include <algorithm>
#include <random>

namespace
{
    thread_local std::mt19937 rng{std::random_device{}()};

    int physicalDamage(int attackerAttack, int targetDefense)
    {
        int raw = std::max(1, attackerAttack - targetDefense / 2);
        std::uniform_int_distribution<int> dist(-raw / 20, raw / 20);
        return std::max(1, raw + dist(rng));
    }

    bool rollFleeSuccess(int playerSpeed, int enemySpeed)
    {
        int chance = 50 + (playerSpeed - enemySpeed) * 2;
        chance = std::clamp(chance, 10, 90);
        std::uniform_int_distribution<int> dist(1, 100);
        return dist(rng) <= chance;
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
    enemies_ = std::move(enemies);
    log_.clear();
    playerGuarding_ = false;
    fled_ = false;

    appendLog("Battle started: " + player.name() + " vs shadows");
    for (const auto &enemy : enemies_)
    {
        if (enemy)
            appendLog(enemy->battleCry());
    }
}

bool BattleSystem::playerAttack(size_t enemyIndex)
{
    if (isOver())
        return false;
    Enemy *target = enemyAt(enemyIndex);
    if (!target || !target->isAlive())
        return false;

    int damage = physicalDamage(player_->attack(), target->defense());
    target->takeDamage(damage);
    appendLog(player_->name() + " attacks " + target->name() + " for " +
              std::to_string(damage) + " damage.");

    enemyTurn();
    return true;
}

bool BattleSystem::playerUseSkill(size_t enemyIndex, const Skill &skill)
{
    if (isOver())
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

    enemyTurn();
    return true;
}

bool BattleSystem::playerUseItem(size_t inventoryIndex)
{
    if (isOver())
        return false;
    // Item usage is handled by the caller (InventoryScene / GameManager) because
    // BattleSystem does not own the inventory. This stub remains for interface symmetry.
    appendLog(player_->name() + " uses an item.");
    enemyTurn();
    return true;
}

bool BattleSystem::playerGuard()
{
    if (isOver())
        return false;
    playerGuarding_ = true;
    appendLog(player_->name() + " takes a defensive stance.");
    enemyTurn();
    return true;
}

bool BattleSystem::playerSwitchPersona(std::shared_ptr<Persona> persona)
{
    if (isOver() || !persona)
        return false;
    player_->setPersona(persona);
    appendLog(player_->name() + " switches to " + persona->name() + ".");
    enemyTurn();
    return true;
}

bool BattleSystem::playerFlee()
{
    if (isOver())
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
    enemyTurn();
    return false;
}

void BattleSystem::enemyTurn()
{
    if (isOver())
        return;

    playerGuarding_ = false;

    for (auto &enemy : enemies_)
    {
        if (!enemy || !enemy->isAlive())
            continue;

        auto skill = enemy->chooseSkill();
        if (skill && !skill->isHealing())
        {
            int raw = (skill->power() * enemy->attack()) / (player_->defense() + 10);
            std::uniform_int_distribution<int> dist(-raw / 20, raw / 20);
            int actual = std::max(1, raw + dist(rng));
            actual = applyAffinityMultiplier(actual, player_->affinity(skill->element()));

            if (playerGuarding_)
                actual /= 2;

            player_->takeDamage(actual);
            appendLog(enemy->name() + " casts " + skill->name() + " for " +
                      std::to_string(actual) + " damage.");
        }
        else
        {
            int actual = physicalDamage(enemy->attack(), player_->defense());
            if (playerGuarding_)
                actual /= 2;
            player_->takeDamage(actual);
            appendLog(enemy->name() + " attacks " + player_->name() + " for " +
                      std::to_string(actual) + " damage.");
        }

        if (!player_->isAlive())
            break;
    }
}

bool BattleSystem::isOver() const
{
    return fled_ || !player_->isAlive() ||
           std::none_of(enemies_.begin(), enemies_.end(),
                        [](const auto &e) { return e && e->isAlive(); });
}

bool BattleSystem::playerWon() const
{
    return player_->isAlive() &&
           std::none_of(enemies_.begin(), enemies_.end(),
                        [](const auto &e) { return e && e->isAlive(); });
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

void BattleSystem::appendLog(const std::string &message)
{
    log_.push_back(message);
}
