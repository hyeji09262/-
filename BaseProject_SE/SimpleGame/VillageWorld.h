#pragma once
#include <array>
#include <cmath>
#include "BuildingVisuals.h"

namespace CoastalWorld
{
constexpr int Width = 60;
constexpr int Height = 192;
constexpr int LandWidth = Width;
constexpr int TownCount = 3;
constexpr int ResidentsPerTown = 10;
constexpr int WildPopulation = 50;
constexpr float WalkSpeed = 3.5f;
constexpr float RespawnSeconds = 3.f;

constexpr float TownCenterY(int town)
{
    return 26.f + town * 70.f;
}

inline const char* TownName(int town)
{
    const char* names[] = {"별빛 마을", "해솔 항구 마을", "달빛 숲 마을"};
    return names[town];
}

inline int NearestTown(float y)
{
    return y < 61 ? 0 : y < 131 ? 1 : 2;
}

inline bool Safe(float x, float y)
{
    for (int i = 0; i < TownCount; ++i)
    {
        if (x >= 10 && x <= 58 && y >= TownCenterY(i) - 15 && y <= TownCenterY(i) + 24)
        {
            return true;
        }
    }
    return false;
}

inline bool Road(int x, int y)
{
    if (std::abs(x - 31) <= 1 && y >= 15 && y <= 179)
    {
        return true;
    }
    for (int town = 0; town < TownCount; ++town)
    {
        const int center = int(TownCenterY(town));
        if (x >= 13 && x <= 42 &&
            (std::abs(y - center) <= 1 || std::abs(y - center - 10) <= 1 ||
             std::abs(y - center - 23) <= 1))
        {
            return true;
        }
        if ((std::abs(x - 22) <= 1 || std::abs(x - 32) <= 1) && y >= center - 12 &&
            y <= center + 24)
        {
            return true;
        }
    }
    return false;
}

struct Building
{
    const char* name;
    float x, y;
    int town;
    int style;
    // 0 house, 1 school, 2 shop, 3 academy, 4 clinic.
    int interior;
};

inline float DoorY(const Building& building)
{
    return building.y + BuildingVisuals::Get(building.style).entranceY;
}

inline float DoorX(const Building& building)
{
    return building.x + BuildingVisuals::Get(building.style).entranceX;
}

inline bool Blocks(const Building& building, float x, float y)
{
    if (building.interior == 5)
    {
        return false;
    }
    const auto& bounds = BuildingVisuals::Get(building.style);
    return std::abs(x - building.x) < bounds.halfWidth &&
           std::abs(y - building.y) < bounds.halfDepth;
}

inline bool SchoolGround(float x, float y)
{
    for (int town = 0; town < TownCount; ++town)
    {
        if (x >= 32 && x <= 43 && y >= TownCenterY(town) - 13 && y <= TownCenterY(town) - 1)
        {
            return true;
        }
    }
    return false;
}

inline const std::array<Building, 21>& Buildings()
{
    static const std::array<Building, 21> data = {{
        {"우리 집", 16, 16, 0, 0, 0},        {"별빛 학교", 38, 16, 0, 4, 1},
        {"별빛 편의점", 16, 29, 0, 5, 2},    {"별빛 병원", 38, 30, 0, 1, 4},
        {"별빛 쇼핑몰", 16, 43, 0, 2, 7},    {"별빛역", 38, 44, 0, 3, 6},
        {"별빛 중앙공원", 49, 32, 0, 8, 5},  {"해솔 주택", 16, 86, 1, 0, 0},
        {"해솔 학교", 38, 86, 1, 4, 1},      {"해솔 편의점", 16, 99, 1, 5, 2},
        {"해솔 병원", 38, 100, 1, 1, 4},     {"해솔 쇼핑몰", 16, 113, 1, 2, 7},
        {"해솔역", 38, 114, 1, 3, 6},        {"해솔 중앙공원", 49, 102, 1, 8, 5},
        {"달빛 주택", 16, 156, 2, 0, 0},     {"달빛 학교", 38, 156, 2, 4, 1},
        {"달빛 편의점", 16, 169, 2, 5, 2},   {"달빛 병원", 38, 170, 2, 1, 4},
        {"달빛 쇼핑몰", 16, 183, 2, 2, 7},   {"달빛역", 38, 184, 2, 3, 6},
        {"달빛 중앙공원", 49, 172, 2, 8, 5},
    }};
    return data;
}

inline bool Paved(int x, int y)
{
    for (int town = 0; town < TownCount; ++town)
    {
        const float center = TownCenterY(town) + 6;
        if (x >= 42 && x <= 57 && std::abs(y - center) <= 1)
        {
            return true;
        }
        if (std::abs(x - 49) <= 1 && std::abs(y - center) <= 7)
        {
            return true;
        }
    }
    if (Road(x, y))
    {
        return true;
    }
    for (const auto& building : Buildings())
    {
        const float door = DoorY(building);
        float street = TownCenterY(building.town);
        while (street < door && street < TownCenterY(building.town) + 23)
        {
            street += street == TownCenterY(building.town) ? 10.f : 13.f;
        }
        if (std::abs(x - DoorX(building)) <= .85f && y >= door - .7f && y <= street + .6f)
        {
            return true;
        }
    }
    return false;
}

struct Resident
{
    const char* name;
    const char* line;
    float x, y;
};

inline const std::array<Resident, 30>& Residents()
{
    static const std::array<Resident, 30> data = {{
        {"담임 모라", "악몽 교감을 물리치고 해안 별빛 장치에서 조각을 찾아주세요.", 32, 24},
        {"학생 소미", "학교 도서관에 재미있는 책이 많아! 체육관 쪽은 오늘 좀 이상해.", 34, 25},
        {"학생 서진", "집과 학교는 E로 들어갈 수 있어. 학교 안에서 이야기를 이어 가자.", 29, 25},
        {"반장 루아", "Q 공격, Z 체력 물약, X 마나 물약! R 충전은 이제 없어.", 33, 22},
        {"방송부 무영", "방송실에서 괴물 목소리가 들려. 오디션은 끝났는데.", 28, 24},
        {"경비원 단", "마을 안은 안전해. 바깥 숲에서는 길을 따라 이동해.", 36, 26},
        {"보건교사 연두", "보건소에 들어가 E로 쉬면 체력과 마나가 회복돼.", 31, 29},
        {"학생 철수", "우리 마을에서 해솔까지 걸어서 약 20초쯤 걸려.", 26, 28},
        {"편의점 주인 담", "편의점 안의 보급 상자에서 물약을 받을 수 있어.", 24, 32},
        {"학생회장 은", "F1과 F6은 마을로 돌아오는 키야. 학교 입구에서 이야기를 이어 가자.", 29,
         31},
        {"선장 해솔", "별빛 마을에서 오는 길은 남쪽 해안길이야.", 32, 94},
        {"어부 나루", "바다가 숙제를 가져가 주면 좋겠다는 학생이 있더라.", 34, 95},
        {"항구 학생 보리", "우리 분교에도 도서관과 체육관이 있어.", 29, 95},
        {"음악교사 소리", "음악학원은 파란 간판! 악몽도 음정은 맞춰야지.", 33, 92},
        {"배달원 하루", "마을 사이 길은 막히지 않아. Shift로 달릴 수도 있어.", 28, 94},
        {"항구 경비 수호", "숲의 괴물은 처치하면 다른 곳에서 다시 나타나.", 36, 96},
        {"간호사 채원", "바깥이 불안하면 진료소에서 쉬어 가.", 31, 99},
        {"관광객 미루", "색색의 지붕을 따라 걷다 보니 길을 잃을 틈이 없네.", 26, 98},
        {"상인 파도", "물약은 편의점에서 보급해 드려요. 병은 바다에 버리지 마세요.", 24, 102},
        {"아이 별", "달빛 마을은 미니맵 아래쪽! 해안길을 따라 더 가면 돼.", 29, 101},
        {"숲지기 다온", "여긴 달빛 숲 마을. 나무 사이에 악몽이 숨어 있어.", 32, 164},
        {"화가 아린", "마법소녀의 색은 분홍? 오늘 숲은 라일락색이네.", 34, 165},
        {"학생 누리", "학원도 E로 들어가. 오늘은 정물화 수업이래.", 29, 165},
        {"미술교사 솔", "움직이는 나무는 정물로 인정하지 않아요.", 33, 162},
        {"목수 건우", "집마다 창문과 지붕을 다르게 지었어.", 28, 164},
        {"산림 경비 라온", "괴물 수가 많을수록 마을 분위기가 불안해져.", 36, 166},
        {"의사 지안", "먼 길을 왔네. 보건소에 들렀다가 숲을 조사해.", 31, 169},
        {"연구원 도윤", "마을별 안전도와 주변 몬스터 수를 모니터에서 확인할 수 있어.", 26, 168},
        {"상인 모루", "숲에서도 편의점은 열어요. 악몽 할인은 없습니다.", 24, 172},
        {"학생 하늘", "해솔 마을을 지나면 별빛 학교로 돌아갈 수 있어.", 29, 171},
    }};
    return data;
}
} // namespace CoastalWorld
