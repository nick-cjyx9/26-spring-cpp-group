#include "Enemy.h"

#include <algorithm>

Enemy::Enemy(std::string id, std::string name, int hp, int attack, int defense,
             int rewardExp, int rewardGold)
    : id_(std::move(id)), name_(std::move(name)), hp_(hp), maxHp_(hp),
      attack_(attack), defense_(defense), rewardExp_(rewardExp), rewardGold_(rewardGold) {}

void Enemy::takeDamage(int damage)
{
    int actual = std::max(1, damage - defense_);
    hp_ -= actual;
    if (hp_ < 0)
        hp_ = 0;
}

Slime::Slime() : Enemy("enemy_slime", "Slime", 30, 8, 2, 15, 5) {}

Goblin::Goblin() : Enemy("enemy_goblin", "Goblin", 50, 12, 4, 30, 10) {}

Boss::Boss() : Enemy("enemy_boss", "Campus Guardian", 120, 20, 8, 100, 50) {}
