// CPU-only regression checks; no game window, OpenGL context or save file writes.
#include <cassert>
#include <queue>
#include "../SimpleGame/QuestCombat.h"
#include "../SimpleGame/SchoolScene.h"

int main()
{
    using MainQuest::Point;
    for (int sector = 0; sector < 8; ++sector)
    {
        const float angle = sector * .7853981634f;
        const Point facing{std::cos(angle), std::sin(angle)};
        assert(CharacterVisual::FacingSector(facing.x, facing.y) == sector);
        MainQuest::State state;
        state.active = true;
        state.step = MainQuest::Step::Shadows;
        MainQuest::Combat combat;
        combat.Enter(state);
        // An enemy behind the player must not turn the cast pose or projectile around.
        for (auto& enemy : combat.enemies)
        {
            enemy.health = 0;
        }
        combat.enemies[0].health = 10;
        combat.enemies[0].position = {10 - facing.x, 10 - facing.y};
        combat.Attack({10, 10}, facing, state);
        assert(combat.shots.size() == 1);
        const auto& shot = combat.shots.front();
        assert(std::abs(shot.direction.x - facing.x) < .0001f);
        assert(std::abs(shot.direction.y - facing.y) < .0001f);
        assert(std::abs(combat.castDirection.x - shot.direction.x) < .0001f);
        assert(std::abs(combat.castDirection.y - shot.direction.y) < .0001f);
        assert(std::abs(combat.mana - 26) < .0001f);
        combat.attackCooldown = 0;
        combat.enemies.clear();
        combat.Attack({10, 10}, facing, state);
        assert(combat.shots.size() == 2);
        assert(std::abs(combat.shots.back().direction.x - facing.x) < .0001f);
    }

    AutoTarget::Selection selection(0, 0, 1, 0, 7);
    assert(!selection.Consider(1, 0, false));
    assert(!selection.Consider(1, 0, true, false));
    assert(!selection.Consider(8, 0, true));
    assert(selection.Consider(4, 0, true));
    assert(!selection.Consider(0, 2, true));
    assert(selection.Consider(3, 0, true));
    assert(selection.directionX == 1 && selection.directionY == 0);

    for (int sector = 0; sector < 8; ++sector)
    {
        const float facing = sector * .7853981634f;
        float fx = std::cos(facing) + 2 * std::sin(facing);
        float fy = 2 * std::sin(facing) - std::cos(facing);
        const float facingLength = std::hypot(fx, fy);
        fx /= facingLength;
        fy /= facingLength;
        for (float offset : {-31.f, -30.f, -20.f, 0.f, 20.f, 30.f, 31.f, 90.f, 180.f})
        {
            AutoTarget::Selection aim(2, 3, fx, fy, 7);
            const float angle = facing + offset * .01745329252f;
            const float tx = std::cos(angle) + 2 * std::sin(angle);
            const float ty = 2 * std::sin(angle) - std::cos(angle);
            const float targetLength = std::hypot(tx, ty);
            const bool accepted =
                aim.Consider(2 + 4 * tx / targetLength, 3 + 4 * ty / targetLength, true);
            // Avoid float rounding exactly on the inclusive 30-degree boundary.
            if (std::abs(offset) != 30.f)
            {
                assert(accepted == (std::abs(offset) < 30.f));
            }
            if (!accepted)
            {
                assert(std::abs(aim.directionX - fx) < .0001f);
                assert(std::abs(aim.directionY - fy) < .0001f);
            }
        }
    }
    // Selecting one target must not rotate the cone and admit a second target outside it.
    AutoTarget::Selection anchored(0, 0, 1, 0, 7);
    assert(anchored.Consider(4, 1, true));
    assert(!anchored.Consider(2, 2, true));
    assert(anchored.Consider(2, -.5f, true));

    std::array<float, 32> sockets{};
    for (int sector = 0; sector < 8; ++sector)
    {
        sockets[sector * 4] = 10.f + sector;
        sockets[sector * 4 + 1] = -30.f;
        sockets[sector * 4 + 2] = 20.f + sector;
        sockets[sector * 4 + 3] = -50.f;
    }
    CharacterVisual::SetDirectionSockets(sockets);
    for (int sector = 0; sector < 8; ++sector)
    {
        const float x = std::cos(sector * .7853981634f);
        const float y = std::sin(sector * .7853981634f);
        for (float tileWidth : {32.f, 40.f})
        {
            const auto socket = CharacterVisual::FacingSocket(true, x, y, tileWidth);
            assert(std::abs((socket.x - socket.y) * tileWidth - (20 + sector)) < .001f);
            const auto spell = CharacterVisual::FacingSpellPose(4, 7, 0, x, y, tileWidth);
            assert(std::abs(spell.x - 4 - socket.x) < .001f);
            assert(std::abs(spell.y - 7 - socket.y) < .001f);
            assert(std::abs(spell.z - socket.z) < .001f);
            for (float age : {.05f, .18f, .5f})
            {
                const auto flying = CharacterVisual::FacingSpellPose(
                    4 + x * age * 9, 7 + y * age * 9, age, x, y, tileWidth);
                assert(std::abs(flying.x - spell.x - x * age * 9) < .001f);
                assert(std::abs(flying.y - spell.y - y * age * 9) < .001f);
                assert(std::abs(flying.z - spell.z) < .001f);
            }
        }
    }

    // Flood the actual collision layout at half-tile resolution. Every room's entrance,
    // exit and central interaction spot must remain connected around desks and shelves.
    for (int room = 0; room < 8; ++room)
    {
        const auto& bounds = SchoolLayout::Get(room);
        const int width = bounds.width * 2 + 1;
        const int height = bounds.height * 2 + 1;
        std::vector<bool> reached(width * height);
        std::queue<std::pair<int, int>> pending;
        const int startX = bounds.width;
        const int startY = bounds.height * 2 - 6;
        assert(SchoolLayout::Walkable(room, startX * .5f, startY * .5f));
        reached[startY * width + startX] = true;
        pending.push({startX, startY});
        const int steps[][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        while (!pending.empty())
        {
            const auto point = pending.front();
            pending.pop();
            for (const auto& step : steps)
            {
                const int x = point.first + step[0];
                const int y = point.second + step[1];
                if (x >= 0 && y >= 0 && x < width && y < height && !reached[y * width + x] &&
                    SchoolLayout::Walkable(room, x * .5f, y * .5f))
                {
                    reached[y * width + x] = true;
                    pending.push({x, y});
                }
            }
        }
        assert(reached[(bounds.height * 2 - 3) * width + bounds.width]);
        assert(reached[8 * width + bounds.width]);
        if (room == 0)
        {
            for (const auto& door : SchoolLayout::Doors())
            {
                assert(reached[int(door.y * 2) * width + int(door.x * 2)]);
            }
        }
        Game::SceneGraph graph;
        graph.BeginSync();
        SchoolScene::Build(graph, room,
                           [](Hunting::ModelId, const Game::Transform&)
                           {
                           });
        graph.EndSync();
        graph.BeginSync();
        graph.EndSync();
        assert(graph.Find("school/interior"));
        assert(graph.Find("school/floor/0"));
        assert(graph.Find("school/fixture/0"));
    }
}
