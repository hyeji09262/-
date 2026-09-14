#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace Hunting
{
struct Position
{
    float x = 0;
    float y = 0;
};

enum class Tile : unsigned char
{
    Ground,
    Water,
    Tree,
    Rock
};

class Map
{
  public:
    static constexpr int Width = 60;
    static constexpr int Height = 48;
    static constexpr int Count = Width * Height;
    std::array<Tile, Count> tiles;
    std::uint32_t seed = 0;
    Position camp = {30.5f, 24.5f};

    void Generate(std::uint32_t newSeed);
    bool Floor(int x, int y) const;
    bool Walkable(Position p, float radius = 0.23f) const;
    bool Safe(Position p) const;
    void Move(Position& p, Position delta) const;
    std::array<int, Count> Distances(Position origin) const;
    bool AllFloorConnected() const;
    std::vector<Position> SpawnLocations() const;
    static int Index(int x, int y);
};

float Distance(Position a, Position b);
} // namespace Hunting
