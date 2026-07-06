#pragma once

#include <string>

// How an entity reacts to a given Element.
enum class Affinity
{
    Weak,   // 1.5x damage, target knocked Down, attacker gains 1 More
    Normal, // 1.0x damage
    Resist, // 0.5x damage, no Down
    Null,   // 0 damage, no Down
    Drain,  // Absorb as healing, no Down, attacker does not gain 1 More
    Repel   // Reflect damage back, no Down
};

inline std::string affinityName(Affinity a)
{
    switch (a)
    {
    case Affinity::Weak:
        return "Weak";
    case Affinity::Normal:
        return "Normal";
    case Affinity::Resist:
        return "Resist";
    case Affinity::Null:
        return "Null";
    case Affinity::Drain:
        return "Drain";
    case Affinity::Repel:
        return "Repel";
    default:
        return "Unknown";
    }
}
