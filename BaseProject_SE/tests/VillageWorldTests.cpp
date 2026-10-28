// Standalone C++17 layout checks. No renderer, OpenGL, game launch or save files needed.
#include <cassert>
#include <array>
#include <cmath>
#include <queue>
#include <utility>
#include "../SimpleGame/VillageWorld.h"

int main()
{
    using namespace CoastalWorld;
    assert(Buildings().size() == 21);
    assert(Residents().size() == TownCount * ResidentsPerTown);
    assert(WildPopulation == 50);
    assert(RespawnSeconds == 3.f);
    std::array<int, TownCount> population{};
    for (const auto& npc : Residents())
    {
        ++population[NearestTown(npc.y)];
    }
    for (int town = 0; town < TownCount; ++town)
    {
        assert(population[town] == ResidentsPerTown);
        assert(Safe(31, TownCenterY(town)));
        if (town > 0)
        {
            assert(std::abs((TownCenterY(town) - TownCenterY(town - 1)) / WalkSpeed - 20.f) <
                   .001f);
        }
    }

    // Forest placement excludes the entire main road; no building may block it either.
    for (int y = 26; y <= 166; ++y)
    {
        assert(Road(31, y));
        for (const auto& building : Buildings())
        {
            assert(!(std::abs(31 - building.x) < 1.25f && std::abs(y - building.y) < .9f));
        }
    }

    // All building thresholds and resident positions must connect on the authored land layout.
    auto walkable = [](int x, int y)
    {
        if (x < 1 || x >= LandWidth || y < 1 || y >= Height - 1)
        {
            return false;
        }
        for (const auto& building : Buildings())
        {
            if (std::abs(x - building.x) < 1.25f && std::abs(y - building.y) < .9f)
            {
                return false;
            }
        }
        return true;
    };
    std::array<bool, Width * Height> reached{};
    std::queue<std::pair<int, int>> pending;
    pending.push({31, 26});
    reached[26 * Width + 31] = true;
    const int directions[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!pending.empty())
    {
        const auto position = pending.front();
        pending.pop();
        for (const auto& direction : directions)
        {
            const int x = position.first + direction[0];
            const int y = position.second + direction[1];
            if (walkable(x, y) && !reached[y * Width + x])
            {
                reached[y * Width + x] = true;
                pending.push({x, y});
            }
        }
    }
    for (const auto& building : Buildings())
    {
        assert(building.style >= 0 && building.style < 9);
        assert(reached[(int(building.y) + 2) * Width + int(building.x)]);
    }
    for (const auto& npc : Residents())
    {
        assert(reached[int(npc.y) * Width + int(npc.x)]);
    }
}
