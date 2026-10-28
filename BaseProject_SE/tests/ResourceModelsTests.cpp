#include <cassert>
#include <cmath>
#include <queue>
#include <vector>
#include "../SimpleGame/PrimitiveModels.h"
#include "../SimpleGame/CharacterVisual.h"
#include "../SimpleGame/VillageWorld.h"

int main()
{
    Hunting::PrimitiveModels models;
    models.Initialize();
    assert(models.loadedFromResource);
    assert(CharacterVisual::HasDirectionSockets());
    // Check building footprints and entrance planters at half-tile resolution.
    const int width = CoastalWorld::LandWidth * 2;
    const int height = CoastalWorld::Height * 2;
    auto walkable = [](float x, float y)
    {
        for (const auto& building : CoastalWorld::Buildings())
        {
            if (CoastalWorld::Blocks(building, x, y))
            {
                return false;
            }
            const auto& style = BuildingVisuals::Get(building.style);
            for (float side : {-1.f, 1.f})
            {
                if (std::hypot(x - building.x - side * (style.halfWidth - .5f),
                               y - building.y - style.halfDepth - .3f) < .62f)
                {
                    return false;
                }
            }
        }
        return true;
    };
    std::vector<bool> reached(width * height);
    std::queue<std::pair<int, int>> pending;
    pending.push({62, 52});
    reached[52 * width + 62] = true;
    const int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    while (!pending.empty())
    {
        const auto point = pending.front();
        pending.pop();
        for (const auto& step : steps)
        {
            const int x = point.first + step[0];
            const int y = point.second + step[1];
            if (x > 0 && y > 0 && x < width && y < height && !reached[y * width + x] &&
                walkable(x * .5f, y * .5f))
            {
                reached[y * width + x] = true;
                pending.push({x, y});
            }
        }
    }
    for (const auto& building : CoastalWorld::Buildings())
    {
        const auto& style = BuildingVisuals::Get(building.style);
        assert(style.halfWidth > 1 && style.halfDepth > 1);
        assert(style.labelHeight > 100);
        const int doorX = int(std::round(CoastalWorld::DoorX(building) * 2));
        const int doorY = int(std::round(CoastalWorld::DoorY(building) * 2));
        assert(reached[doorY * width + doorX]);
        assert(CoastalWorld::Blocks(building, building.x, building.y) == (building.interior != 5));
        assert(!CoastalWorld::Blocks(building, CoastalWorld::DoorX(building),
                                     CoastalWorld::DoorY(building)));
        for (const auto& other : CoastalWorld::Buildings())
        {
            assert(!CoastalWorld::Blocks(other, CoastalWorld::DoorX(building),
                                         CoastalWorld::DoorY(building)));
        }
    }
    for (int model = 0; model < static_cast<int>(Hunting::ModelId::Count); ++model)
    {
        const auto& mesh = models.Get(static_cast<Hunting::ModelId>(model));
        assert(!mesh.empty() && mesh.size() % 3 == 0);
    }
    for (int direction = 0; direction < 8; ++direction)
    {
        const float x = std::cos(direction * .7853981634f);
        const float y = std::sin(direction * .7853981634f);
        for (int frame = 0; frame < 6; ++frame)
        {
            const auto model = static_cast<Hunting::ModelId>(frame);
            const auto& mesh = models.GetFacing(model, x, y);
            assert(!mesh.empty());
            assert(&mesh == &models.GetFacing(model, x, y));
            assert(&mesh != &models.Get(model));
        }
        const auto socket = CharacterVisual::FacingSocket(true, x, y);
        const auto spell = CharacterVisual::FacingSpellPose(0, 0, 0, x, y);
        assert(std::abs(socket.x - spell.x) < .0001f);
        assert(std::abs(socket.y - spell.y) < .0001f);
        assert(std::abs(socket.z - spell.z) < .0001f);
    }
}
