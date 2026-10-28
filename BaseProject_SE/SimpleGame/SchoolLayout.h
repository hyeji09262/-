#pragma once
#include <array>
#include <cmath>
#include <vector>
#include "PrimitiveModels.h"
#include "VillageInterior.h"

namespace SchoolLayout
{
constexpr float IntroWindowX = 9.f, IntroWindowY = 1.35f;
constexpr float IntroPlayerX = 10.f, IntroPlayerY = 9.f;
constexpr float IntroLumiX = 10.f, IntroLumiY = 6.f;

struct Room
{
    const char* name;
    int width, height;
};

inline const Room& Get(int room)
{
    static const std::array<Room, 8> rooms{{{"중앙 복도", 36, 14},
                                            {"도서관", 24, 20},
                                            {"체육관", 32, 26},
                                            {"방송실", 18, 16},
                                            {"1학년 1반", 24, 20},
                                            {"1학년 2반", 24, 20},
                                            {"과학실", 26, 20},
                                            {"미술실", 26, 20}}};
    return rooms[room];
}

struct Door
{
    int room;
    float x, y;
};

inline const std::array<Door, 7>& Doors()
{
    static const std::array<Door, 7> doors{
        {{4, 4, 3}, {5, 9, 3}, {6, 14, 3}, {7, 19, 3}, {1, 24, 3}, {3, 29, 3}, {2, 33, 3}}};
    return doors;
}

struct Fixture
{
    Hunting::ModelId model;
    float x, y, scale = 1;
    float radius = .7f;
};

inline const std::vector<Fixture>& Fixtures(int room)
{
    using Hunting::ModelId;
    static const auto rooms = []()
    {
        std::array<std::vector<Fixture>, 8> data;
        data[0] = {{ModelId::Bench, 7, 11},
                   {ModelId::Bench, 27, 11},
                   {ModelId::InteriorLockerView2, 6, 2.8f}};
        data[1] = {{ModelId::Bookshelf, 3, 4},  {ModelId::Bookshelf, 8, 4},
                   {ModelId::Bookshelf, 16, 4}, {ModelId::Bookshelf, 21, 4},
                   {ModelId::Bookshelf, 3, 8},  {ModelId::Bookshelf, 21, 8},
                   {ModelId::Desk, 7, 12},      {ModelId::Chair, 7, 13.5f, 1, .45f},
                   {ModelId::Desk, 17, 12},     {ModelId::Chair, 17, 13.5f, 1, .45f}};
        data[2] = {{ModelId::Bench, 3, 8},
                   {ModelId::Bench, 3, 14},
                   {ModelId::Bench, 29, 8},
                   {ModelId::Bench, 29, 14}};
        data[3] = {{ModelId::Desk, 4, 5},           {ModelId::InteriorComputerView0, 4, 5, 1, 0},
                   {ModelId::Cabinet, 8, 5},        {ModelId::Desk, 13, 5},
                   {ModelId::Chair, 4, 7, 1, .45f}, {ModelId::Chair, 13, 7, 1, .45f},
                   {ModelId::Bookshelf, 2, 10}};
        for (int classroom = 4; classroom < 8; ++classroom)
        {
            auto& fixtures = data[classroom];
            if (classroom == 4 || classroom == 5)
            {
                for (const auto& item : VillageInterior::Classroom())
                {
                    const int index = int(item.model) - int(ModelId::InteriorBedView0);
                    const auto rotated =
                        static_cast<ModelId>(int(item.model) - index % 4 + (index % 4 + 2) % 4);
                    const bool board = item.model == ModelId::InteriorWhiteboardView0;
                    fixtures.push_back(
                        {rotated, 22.f - item.x, board ? 2.3f : 19.f - item.y, 1, item.radius});
                }
                continue;
            }
            fixtures.push_back({ModelId::Blackboard, 12, 2.3f, 1.4f, .5f});
            fixtures.push_back({ModelId::Desk, 4, 3.5f, 1, .8f});
            for (int row = 0; row < 3; ++row)
            {
                for (int column = 0; column < 5; ++column)
                {
                    const float x = 4.f + column * 4;
                    const float y = 7.f + row * 3;
                    fixtures.push_back({ModelId::Desk, x, y, 1, .85f});
                    fixtures.push_back({ModelId::Chair, x, y + 1.25f, .85f, .35f});
                    if (classroom >= 6)
                    {
                        fixtures.push_back({classroom == 6 ? ModelId::Potion : ModelId::Book,
                                            x + .3f, y - .15f, .6f, 0});
                    }
                }
            }
            fixtures.push_back({ModelId::Bookshelf, 2, 17});
            fixtures.push_back({ModelId::Cabinet, 22, 17});
        }
        return data;
    }();
    return rooms[room];
}

inline bool Walkable(int room, float x, float y)
{
    const auto& bounds = Get(room);
    if (x < .8f || x > bounds.width - .8f || y < 2 || y > bounds.height - .8f)
    {
        return false;
    }
    for (const auto& fixture : Fixtures(room))
    {
        if (fixture.radius > 0 && std::hypot(x - fixture.x, y - fixture.y) < fixture.radius + .22f)
        {
            return false;
        }
    }
    return true;
}
} // namespace SchoolLayout
