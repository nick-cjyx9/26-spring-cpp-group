#pragma once

enum class TileType
{
    Floor,
    Wall
};

class Tile
{
public:
    Tile() = default;
    explicit Tile(TileType type) : type_(type) {}

    TileType type() const { return type_; }
    bool isWalkable() const { return type_ == TileType::Floor; }

private:
    TileType type_ = TileType::Floor;
};
