// Standalone CPU-only tests. Build separately, without the game's precompiled header.
#include <cassert>
#include <cmath>
#include <vector>
#include "../SimpleGame/SceneGraph.h"
#include "../SimpleGame/CharacterVisual.h"

int main()
{
    using Game::RenderLayer;
    Game::SceneGraph graph;
    graph.BeginSync();
    auto& root = graph.Place("root", {10, 20, 3, 2}, RenderLayer::World);
    auto& child = graph.Place("child", {1, -2, 5, .5f}, RenderLayer::World, {}, &root);
    auto world = child.WorldTransform();
    assert(world.x == 12 && world.y == 16 && world.z == 13 && world.scale == 1);
    root.visible = false;
    assert(!child.IsVisible());
    root.visible = true;
    root.enabled = false;
    assert(!child.IsVisible());
    root.enabled = true;

    bool rejected = false;
    try
    {
        graph.SetParent(root, &child);
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    assert(rejected && root.Parent() == nullptr);
    graph.SetParent(child, nullptr);
    assert(root.Children().empty() && child.WorldTransform().x == 1);
    graph.SetParent(child, &root);
    graph.EndSync();

    // Updating a named actor retains its identity and visibility controls.
    graph.BeginSync();
    auto* savedRoot = &root;
    assert(&graph.Place("root", {}, RenderLayer::World) == savedRoot);
    graph.EndSync();
    assert(graph.Find("child") == nullptr && root.Children().empty());

    graph.Clear();
    graph.BeginSync();
    std::vector<int> order;
    auto draw = [&order](int value)
    {
        return [&order, value](const Game::Actor&, const Game::Transform&)
        {
            order.push_back(value);
        };
    };
    graph.Place("front", {5, 5, 100}, RenderLayer::World, draw(3));
    graph.Place("back", {0, 0}, RenderLayer::World, draw(1));
    graph.Place("tie", {0, 0, 200}, RenderLayer::World, draw(2));
    graph.Place("ground", {99, 99}, RenderLayer::Ground, draw(0));
    graph.Place("ui", {}, RenderLayer::UserInterface, draw(4));
    graph.EndSync();
    graph.Render(RenderLayer::Ground, RenderLayer::World);
    assert((order == std::vector<int>{0, 1, 2, 3}));
    graph.Render(RenderLayer::UserInterface, RenderLayer::UserInterface);
    assert(order.back() == 4);

    order.clear();
    graph.Render(RenderLayer::World, RenderLayer::World,
                 [](const Game::Transform& pose)
                 {
                     return pose.x < 1;
                 });
    assert((order == std::vector<int>{1, 2}));

    auto& terrain = graph.Place("terrain", {}, RenderLayer::Ground);
    auto& tile = graph.Place("tile", {}, RenderLayer::Ground, {}, &terrain);
    auto* tileIdentity = &tile;
    graph.RetainSubtree(terrain);
    graph.BeginSync();
    graph.EndSync();
    assert(graph.Find("tile") == tileIdentity);
    assert(graph.Find("front") == nullptr);
    graph.Remove("terrain");
    assert(graph.Find("tile") == nullptr);

    auto& owner = graph.Place("owner", {}, RenderLayer::World);
    auto& socket = graph.Place("socket", {}, RenderLayer::World, {}, &owner);
    graph.Place("flash", {}, RenderLayer::World, {}, &socket);
    graph.Remove("owner");
    assert(!graph.Find("owner") && !graph.Find("socket") && !graph.Find("flash"));

    for (bool mirror : {false, true})
    {
        for (bool casting : {false, true})
        {
            auto wand = CharacterVisual::WandSocket(casting, mirror);
            float screenX = (wand.x - wand.y) * 32;
            float screenY = (wand.x + wand.y) * 16 - wand.z;
            float hand = casting ? CharacterVisual::CastHandY : CharacterVisual::IdleHandY;
            assert(std::abs(screenX - (mirror ? -20.f : 20.f)) < .001f);
            assert(std::abs(screenY - hand - CharacterVisual::WandHeadOffset) < .001f);
        }
        auto socket = CharacterVisual::WandSocket(true, mirror);
        auto start = CharacterVisual::SpellPose(10, 20, 0, mirror);
        assert(std::abs(start.x - 10 - socket.x) < .001f);
        assert(std::abs(start.y - 20 - socket.y) < .001f);
        assert(start.z == socket.z);
        auto flying = CharacterVisual::SpellPose(10, 20, .18f, mirror);
        assert(flying.x == 10 && flying.y == 20 && flying.z == 27);
    }
    assert(CharacterVisual::FrameIndex(CharacterVisual::CastDuration, true, 2) == 5);
    assert(CharacterVisual::FrameIndex(0, false, 2) == 0);
    assert(CharacterVisual::FrameIndex(0, true, 2) == 3);
}
