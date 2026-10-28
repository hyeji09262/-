#include <cassert>
#include <queue>
#include "../SimpleGame/VillageInterior.h"
#include "../SimpleGame/TownPopulation.h"
#include "../SimpleGame/VillageWorld.h"

int main()
{
    for (int i = 0; i < 10; ++i)
    {
        const auto& p = TownPopulation::ResidentPositions[i];
        for (int j = 0; j < i; ++j)
        {
            const auto& q = TownPopulation::ResidentPositions[j];
            assert(std::hypot(p[0] - q[0], p[1] - q[1]) >= 5);
        }
        for (const auto& building : CoastalWorld::Buildings())
        {
            assert(!CoastalWorld::Blocks(building, p[0],
                                         p[1] + CoastalWorld::TownCenterY(building.town)));
        }
    }
    for (int kind : {0, 2, 3, 4, 6, 7})
    {
        constexpr int width = VillageInterior::Width * 2;
        constexpr int height = VillageInterior::Height * 2;
        std::vector<bool> seen(width * height);
        std::queue<std::pair<int, int>> pending;
        pending.push({20, 30});
        seen[30 * width + 20] = true;
        const int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!pending.empty())
        {
            const auto p = pending.front();
            pending.pop();
            for (const auto& step : steps)
            {
                const int x = p.first + step[0], y = p.second + step[1];
                if (x < 0 || y < 0 || x >= width || y >= height || seen[y * width + x] ||
                    !VillageInterior::Walkable(kind, x * .5f, y * .5f))
                    continue;
                seen[y * width + x] = true;
                pending.push({x, y});
            }
        }
        assert(seen[33 * width + 20]); // Exit.
        assert(seen[int(VillageInterior::ServiceY(kind) * 2) * width + 28]);
        if (kind == 0)
        {
            assert(seen[int((VillageInterior::HomeTvY + 1.5f) * 2) * width + int(VillageInterior::HomeTvX * 2)]);
        }
    }
}
