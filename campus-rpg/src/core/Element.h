#pragma once

#include <string>

// Combat elements used by Persona 4-style weakness / resistance system.
enum class Element
{
    Physical,
    Fire,
    Ice,
    Electric,
    Wind,
    Light,
    Dark,
    Almighty,
    Count
};

inline std::string elementName(Element e)
{
    switch (e)
    {
    case Element::Physical:
        return "Physical";
    case Element::Fire:
        return "Fire";
    case Element::Ice:
        return "Ice";
    case Element::Electric:
        return "Electric";
    case Element::Wind:
        return "Wind";
    case Element::Light:
        return "Light";
    case Element::Dark:
        return "Dark";
    case Element::Almighty:
        return "Almighty";
    default:
        return "Unknown";
    }
}
