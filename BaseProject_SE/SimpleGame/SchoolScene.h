#pragma once
#include "SchoolLayout.h"
#include "SceneGraph.h"

namespace SchoolScene
{
// Shared static school kit. Each retained fixture is an Actor; the caller owns projection.
template <typename DrawModel, typename Graphics, typename Project>
void Build(Game::SceneGraph& graph, int room, DrawModel draw, Graphics& renderer, Project project)
{
    using Game::RenderLayer;
    using Hunting::ModelId;
    Performance::Scope scope("cpu.school.static_scene_build_ms");
    const auto& bounds = SchoolLayout::Get(room);
    auto& root = graph.Place("school/interior", {}, RenderLayer::Ground);
    auto place = [&](const std::string& name, Game::Transform pose, ModelId model,
                     RenderLayer layer = RenderLayer::World)
    {
        graph.Place(
            name, pose, layer,
            [draw, model](const Game::Actor&, const Game::Transform& world)
            {
                draw(model, world);
            },
            &root);
    };
    for (int y = 0; y < bounds.height; ++y)
    {
        for (int x = 0; x < bounds.width; ++x)
        {
            place("school/floor/" + std::to_string(y * bounds.width + x), {x + .5f, y + .5f},
                  room == 0 ? ModelId::FloorStone : ModelId::FloorWood, RenderLayer::Ground);
        }
    }
    for (int x = 1; x < bounds.width; x += 2)
    {
        place("school/wall/" + std::to_string(x), {float(x), .8f}, ModelId::Wall);
        // Classroom entrances occupy the corridor wall. Windows belong inside rooms.
        if (room != 0 && x % 4 == 1)
        {
            graph.Place(
                "school/window/" + std::to_string(x),
                {float(x), SchoolLayout::IntroWindowY, 42}, RenderLayer::World,
                [&renderer, project](const Game::Actor&, const Game::Transform& world)
                {
                    const auto panel = [&](float left, float right, float bottom, float top,
                                           float r, float g, float b)
                    {
                        const auto a = project({world.x + left, world.y, world.z + bottom});
                        const auto c = project({world.x + right, world.y, world.z + bottom});
                        const auto d = project({world.x + right, world.y, world.z + top});
                        const auto e = project({world.x + left, world.y, world.z + top});
                        renderer.Triangle(a.x, a.y, c.x, c.y, d.x, d.y, r, g, b, 1);
                        renderer.Triangle(a.x, a.y, d.x, d.y, e.x, e.y, r, g, b, 1);
                    };
                    panel(-.91f, .91f, 0, 54, .55f, .58f, .67f);
                    panel(-.82f, .82f, 5, 49, .65f, .79f, .86f);
                    panel(-.82f, .82f, 5, 15, .51f, .68f, .75f);
                    panel(-.82f, .82f, 15, 18, .73f, .75f, .81f);
                    panel(-.98f, .98f, -3, 3, .77f, .79f, .84f);
                },
                &root);
        }
    }
    if (room == 0)
    {
        for (const auto& door : SchoolLayout::Doors())
        {
            place("school/door/" + std::to_string(door.room), {door.x, 1.35f, 0, .85f},
                  ModelId::Door);
        }
    }
    else
    {
        place("school/exit", {bounds.width * .5f, bounds.height - .5f, 0, .8f}, ModelId::Door);
    }
    const auto& fixtures = SchoolLayout::Fixtures(room);
    for (size_t i = 0; i < fixtures.size(); ++i)
    {
        const auto& fixture = fixtures[i];
        ModelId model = fixture.model;
        if (model == ModelId::Desk)
            model = ModelId::InteriorStudyDeskView0;
        else if (model == ModelId::Chair)
            model = ModelId::InteriorChairView0;
        else if (model == ModelId::Bookshelf)
            model = ModelId::InteriorBookshelfView0;
        else if (model == ModelId::Cabinet)
            model = ModelId::InteriorLockerView0;
        else if (model == ModelId::Blackboard)
            model = ModelId::InteriorWhiteboardView0;
        const bool tabletop = fixture.radius == 0;
        place("school/fixture/" + std::to_string(i),
              {fixture.x, fixture.y,
               fixture.model == ModelId::Blackboard ? 0.f
               : tabletop                           ? 27.f
                                                    : 0.f,
               fixture.scale},
              model);
    }
    graph.RetainSubtree(root);
    Performance::Profiler::Get().Gauge("school.floor_tiles_count", bounds.width * bounds.height);
    Performance::Profiler::Get().Gauge("school.fixtures_count", double(fixtures.size()));
}
} // namespace SchoolScene
