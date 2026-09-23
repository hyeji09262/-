#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include "HuntingMap.h"
#include "Profiler.h"

namespace Hunting
{
float Distance(Position a, Position b)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

int Map::Index(int x, int y)
{
    return y * Width + x;
}

bool Map::Floor(int x, int y) const
{
    return x >= 0 && x < Width && y >= 0 && y < Height && tiles[Index(x, y)] == Tile::Ground;
}

bool Map::Walkable(Position p, float radius) const
{
    Performance::Profiler::Get().Count("collision.walkable_checks");
    // Four corners enforce clearance for an actor, not just connectivity for a point.
    if (!std::isfinite(p.x) || !std::isfinite(p.y))
    {
        return false;
    }
    for (float dx : {-radius, radius})
    {
        for (float dy : {-radius, radius})
        {
            if (!Floor(static_cast<int>(std::floor(p.x + dx)),
                       static_cast<int>(std::floor(p.y + dy))))
            {
                return false;
            }
        }
    }
    return true;
}

bool Map::Safe(Position p) const
{
    return Distance(p, camp) < 4.f;
}

void Map::Move(Position& p, Position delta) const
{
    Performance::Scope scope("cpu.collision.move_ms");
    // Substeps also prevent tunneling when movement speed is increased later.
    int steps = (std::max)(
        1, static_cast<int>(std::ceil((std::max)(std::abs(delta.x), std::abs(delta.y)) / 0.15f)));
    for (int i = 0; i < steps; ++i)
    {
        Position next = {p.x + delta.x / steps, p.y};
        if (Walkable(next))
        {
            p = next;
        }
        next = {p.x, p.y + delta.y / steps};
        if (Walkable(next))
        {
            p = next;
        }
    }
}

std::array<int, Map::Count> Map::Distances(Position origin) const
{
    Performance::Scope scope("cpu.navigation.bfs_ms");
    Performance::Profiler::Get().Count("navigation.bfs_requests");
    std::array<int, Count> result;
    result.fill(-1);
    int ox = static_cast<int>(origin.x);
    int oy = static_cast<int>(origin.y);
    if (!Floor(ox, oy))
    {
        return result;
    }

    std::queue<int> pending;
    result[Index(ox, oy)] = 0;
    pending.push(Index(ox, oy));
    const int dx[] = {1, -1, 0, 0};
    const int dy[] = {0, 0, 1, -1};
    while (!pending.empty())
    {
        int cell = pending.front();
        pending.pop();
        for (int d = 0; d < 4; ++d)
        {
            int x = cell % Width + dx[d];
            int y = cell / Width + dy[d];
            if (Floor(x, y) && result[Index(x, y)] < 0)
            {
                result[Index(x, y)] = result[cell] + 1;
                pending.push(Index(x, y));
            }
        }
    }
    return result;
}

bool Map::AllFloorConnected() const
{
    const auto distance = Distances(camp);
    for (int i = 0; i < Count; ++i)
    {
        if (tiles[i] == Tile::Ground && distance[i] < 0)
        {
            return false;
        }
    }
    return Walkable(camp);
}

void Map::Generate(std::uint32_t newSeed)
{
    Performance::Scope scope("cpu.map.generate_ms");
    seed = newSeed;
    std::mt19937 random(seed);
    tiles.fill(Tile::Water);
    for (int y = 1; y < Height - 1; ++y)
    {
        for (int x = 1; x < Width - 6; ++x)
        {
            unsigned roll = random() % 100;
            tiles[Index(x, y)] = roll < 13 ? Tile::Tree : roll < 19 ? Tile::Rock : Tile::Ground;
        }
    }

    // Authored safe clearing is the only fixed part of this hunting map.
    for (int y = 19; y <= 29; ++y)
    {
        for (int x = 25; x <= 35; ++x)
        {
            tiles[Index(x, y)] = Tile::Ground;
        }
    }

    // Connect every isolated floor component to camp via a cardinal corridor.
    // Only cells are opened here: an existing connection can never be broken.
    auto distance = Distances(camp);
    for (int i = 0; i < Count; ++i)
    {
        if (tiles[i] != Tile::Ground || distance[i] >= 0)
        {
            continue;
        }
        int x = i % Width;
        int y = i / Width;
        while (x != static_cast<int>(camp.x))
        {
            tiles[Index(x, y)] = Tile::Ground;
            x += x < camp.x ? 1 : -1;
        }
        while (y != static_cast<int>(camp.y))
        {
            tiles[Index(x, y)] = Tile::Ground;
            y += y < camp.y ? 1 : -1;
        }
        distance = Distances(camp);
    }

    // Defensive fallback preserves playability if generation rules are extended incorrectly.
    if (!AllFloorConnected())
    {
        for (int y = 1; y < Height - 1; ++y)
        {
            for (int x = 1; x < Width - 6; ++x)
            {
                tiles[Index(x, y)] = Tile::Ground;
            }
        }
    }
}

std::vector<Position> Map::SpawnLocations() const
{
    std::vector<Position> result;
    for (int y = 2; y < Height - 2; ++y)
    {
        for (int x = 2; x < Width - 7; ++x)
        {
            Position p = {x + 0.5f, y + 0.5f};
            if (Walkable(p) && Distance(p, camp) > 7.f)
            {
                result.push_back(p);
            }
        }
    }
    return result;
}
} // namespace Hunting
