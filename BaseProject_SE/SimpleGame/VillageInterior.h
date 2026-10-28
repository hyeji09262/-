#pragma once
#include <vector>
#include <cmath>
#include "PrimitiveModels.h"
#include "SceneGraph.h"

// Layout coordinates and chair directions imported from
// resource/assets/interiors/layout_catalog.json.
namespace VillageInterior
{
constexpr int Width = 20;
constexpr int Height = 18;
constexpr float HomeTvX = 3.f, HomeTvY = 3.f;
constexpr float ExitX = 10.f, ExitY = 16.5f;

inline float ServiceY(int kind)
{
    return kind == 4 ? 6.5f : 5.f;
}

struct Fixture
{
    Hunting::ModelId model;
    float x, y, radius;
    float z = 0;
};

inline const std::vector<Fixture>& Home()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::Television, HomeTvX, HomeTvY, .8f, 0},
        {M::InteriorSofaView2, 13.00f, 11.60f, 0.80f, 0.00f},
        {M::InteriorDiningTableView0, 13.00f, 7.60f, 0.80f, 0.00f},
        {M::InteriorChairView2, 13.00f, 5.60f, 0.45f, 0.00f},
        {M::InteriorBedView0, 4.80f, 11.60f, 1.10f, 0.00f},
        {M::InteriorWardrobeView0, 4.80f, 3.20f, 0.80f, 0.00f},
        {M::InteriorKitchenUnitView2, 13.60f, 2.40f, 0.80f, 0.00f},
        {M::InteriorFridgeView2, 16.00f, 2.40f, 0.80f, 0.00f},
        {M::InteriorBookshelfView0, 9.20f, 14.00f, 0.80f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& Classroom()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorWhiteboardView0, 10.00f, 14.00f, 0.80f, 0.00f},
        {M::InteriorOfficeDeskView0, 10.00f, 11.50f, 0.80f, 0.00f},
        {M::InteriorStudyDeskView0, 5.20f, 4.40f, 0.80f, 0.00f},
        {M::InteriorChairView2, 5.20f, 2.90f, 0.45f, 0.00f},
        {M::InteriorStudyDeskView0, 5.20f, 8.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 5.20f, 6.50f, 0.45f, 0.00f},
        {M::InteriorStudyDeskView0, 10.00f, 4.40f, 0.80f, 0.00f},
        {M::InteriorChairView2, 10.00f, 2.90f, 0.45f, 0.00f},
        {M::InteriorStudyDeskView0, 10.00f, 8.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 10.00f, 6.50f, 0.45f, 0.00f},
        {M::InteriorStudyDeskView0, 14.80f, 4.40f, 0.80f, 0.00f},
        {M::InteriorChairView2, 14.80f, 2.90f, 0.45f, 0.00f},
        {M::InteriorStudyDeskView0, 14.80f, 8.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 14.80f, 6.50f, 0.45f, 0.00f},
        {M::InteriorLockerView0, 4.00f, 14.00f, 0.80f, 0.00f},
        {M::InteriorLockerView0, 16.00f, 14.00f, 0.80f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& Staffroom()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorOfficeDeskView0, 6.00f, 5.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 6.00f, 3.40f, 0.45f, 0.00f},
        {M::InteriorComputerView0, 6.00f, 5.00f, 0.00f, 27.52f},
        {M::InteriorOfficeDeskView0, 6.00f, 10.60f, 0.80f, 0.00f},
        {M::InteriorChairView2, 6.00f, 9.00f, 0.45f, 0.00f},
        {M::InteriorComputerView0, 6.00f, 10.60f, 0.00f, 27.52f},
        {M::InteriorOfficeDeskView0, 12.00f, 5.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 12.00f, 3.40f, 0.45f, 0.00f},
        {M::InteriorComputerView0, 12.00f, 5.00f, 0.00f, 27.52f},
        {M::InteriorOfficeDeskView0, 12.00f, 10.60f, 0.80f, 0.00f},
        {M::InteriorChairView2, 12.00f, 9.00f, 0.45f, 0.00f},
        {M::InteriorComputerView0, 12.00f, 10.60f, 0.00f, 27.52f},
        {M::InteriorBookshelfView0, 16.00f, 14.00f, 0.80f, 0.00f},
        {M::InteriorBookshelfView0, 13.40f, 14.00f, 0.80f, 0.00f},
        {M::InteriorSofaView0, 6.00f, 14.00f, 0.80f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& Infirmary()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorMedicalBedView0, 5.00f, 11.60f, 1.10f, 0.00f},
        {M::InteriorMedicalBedView0, 8.60f, 11.60f, 1.10f, 0.00f},
        {M::InteriorPrivacyScreenView0, 6.40f, 7.70f, 0.80f, 0.00f},
        {M::InteriorMedicalCartView0, 11.60f, 10.60f, 0.45f, 0.00f},
        {M::InteriorMedicineCabinetView0, 15.80f, 14.00f, 0.80f, 0.00f},
        {M::InteriorOfficeDeskView0, 14.40f, 4.60f, 0.80f, 0.00f},
        {M::InteriorChairView2, 14.40f, 3.00f, 0.45f, 0.00f},
        {M::InteriorComputerView0, 14.40f, 4.60f, 0.00f, 27.52f},
    };
    return items;
}

inline const std::vector<Fixture>& Guardroom()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorOfficeDeskView0, 11.20f, 8.60f, 0.80f, 0.00f},
        {M::InteriorComputerView0, 11.20f, 8.60f, 0.00f, 27.52f},
        {M::InteriorChairView2, 11.20f, 7.00f, 0.45f, 0.00f},
        {M::InteriorLockerView0, 6.60f, 11.20f, 0.80f, 0.00f},
        {M::InteriorSofaView0, 8.60f, 11.00f, 0.80f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& DepartmentStore()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorDisplayTableView0, 6.00f, 6.80f, 0.80f, 0.00f},
        {M::InteriorClothesRackView0, 6.00f, 12.00f, 0.80f, 0.00f},
        {M::InteriorDisplayTableView0, 12.00f, 6.80f, 0.80f, 0.00f},
        {M::InteriorClothesRackView0, 12.00f, 12.00f, 0.80f, 0.00f},
        {M::InteriorCounterView0, 14.00f, 3.40f, 0.80f, 0.00f},
        {M::InteriorCashRegisterView0, 14.00f, 3.40f, 0.00f, 35.20f},
        {M::InteriorMirrorView0, 3.40f, 12.80f, 0.45f, 0.00f},
        {M::InteriorSofaView0, 6.00f, 3.00f, 0.80f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& ConvenienceStore()
{
    using M = Hunting::ModelId;
    static const std::vector<Fixture> items = {
        {M::InteriorGlassFridgeView2, 3.00f, 2.40f, 0.80f, 0.00f},
        {M::InteriorGlassFridgeView2, 6.00f, 2.40f, 0.80f, 0.00f},
        {M::InteriorGlassFridgeView2, 9.00f, 2.40f, 0.80f, 0.00f},
        {M::InteriorStoreShelfView0, 6.00f, 7.00f, 0.80f, 0.00f},
        {M::InteriorStoreShelfView0, 10.00f, 7.00f, 0.80f, 0.00f},
        {M::InteriorStoreShelfView2, 6.00f, 11.00f, 0.80f, 0.00f},
        {M::InteriorStoreShelfView2, 10.00f, 11.00f, 0.80f, 0.00f},
        {M::InteriorCounterView0, 14.00f, 3.40f, 0.80f, 0.00f},
        {M::InteriorCashRegisterView0, 14.00f, 3.40f, 0.00f, 35.20f},
        {M::InteriorDiningTableView0, 16.50f, 11.00f, 0.80f, 0.00f},
        {M::InteriorChairView2, 16.50f, 9.40f, 0.45f, 0.00f},
    };
    return items;
}

inline const std::vector<Fixture>& Fixtures(int kind)
{
    return kind == 2   ? ConvenienceStore()
           : kind == 4 ? Infirmary()
           : kind == 7 ? DepartmentStore()
           : kind == 6 ? Guardroom()
                       : Home();
}

inline bool Walkable(int kind, float x, float y)
{
    if (x < .6f || x > Width - .6f || y < 1 || y > Height - .6f)
        return false;
    for (const auto& item : Fixtures(kind))
        if (item.radius > 0 && std::hypot(x - item.x, y - item.y) < item.radius + .25f)
            return false;
    return true;
}

template <typename DrawModel> void Build(Game::SceneGraph& graph, int kind, DrawModel draw)
{
    Performance::Scope scope("cpu.interior.static_scene_build_ms");
    using Game::RenderLayer;
    using Hunting::ModelId;
    auto& root = graph.Place("inside/floor", {}, RenderLayer::Ground);
    auto place = [&](const std::string& id, Game::Transform pose, ModelId model, RenderLayer layer)
    {
        graph.Place(
            id, pose, layer,
            [draw, model](const Game::Actor&, const Game::Transform& world)
            {
                draw(model, world);
            },
            &root);
    };
    for (int y = 0; y < Height; ++y)
        for (int x = 0; x < Width; ++x)
            place("inside/tile/" + std::to_string(y * Width + x), {x + .5f, y + .5f},
                  kind == 2 ? ModelId::FloorStone : ModelId::FloorWood, RenderLayer::Ground);
    int index = 0;
    for (const auto& fixture : Fixtures(kind))
        place("inside/furniture/" + std::to_string(index++), {fixture.x, fixture.y, fixture.z},
              fixture.model, RenderLayer::World);
    for (int wall = 0; wall < Width; wall += 2)
        place("inside/wall/" + std::to_string(wall), {float(wall), 0}, ModelId::Wall,
              RenderLayer::World);
    graph.RetainSubtree(root);
}
} // namespace VillageInterior
