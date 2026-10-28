#pragma once
#include <array>
#include "PrimitiveModels.h"

namespace TownPopulation
{
constexpr float GuardX = 41.f;
constexpr float GuardY = 21.f;

struct Staff
{
    const char* name;
    const char* line;
    Hunting::ModelId model;
    int kind, room;
    float x, y;
};

inline const std::array<Staff, 6>& StaffMembers()
{
    using M = Hunting::ModelId;
    static const std::array<Staff, 6> members = {
        {{"경비원 단", "학교 앞 길을 살피고 있습니다.", M::Security_GuardView0, -1, 0, GuardX,
          GuardY},
         {"간호사 연두", "접수대에서 쉬면 체력과 마나를 회복할 수 있어요.", M::School_NurseView0, 4,
          0, 17, 4},
         {"백화점 안내원 담", "어서 오세요. 매장별로 둘러보세요.", M::Department_Staff_01View0, 7,
          0, 14, 2},
         {"의류 직원 소라", "의류 코너는 이쪽이에요.", M::Department_Staff_02View0, 7, 0, 5, 10},
         {"생활관 직원 준", "생활용품을 찾으시면 말씀해 주세요.", M::Department_Staff_03View0, 7, 0,
          16, 10},
         {"편의점 알바 나루", "계산대 앞에서 물어보세요. 물약도 준비돼 있어요.",
          M::Convenience_ClerkView0, 2, 0, 14, 2}}};
    return members;
}

constexpr float ResidentPositions[10][2] = {{12, -5}, {24, -10}, {31, -2}, {18, 9},  {26, 16},
                                            {35, 12}, {49, 6},   {55, 12}, {42, 23}, {13, 23}};
} // namespace TownPopulation
