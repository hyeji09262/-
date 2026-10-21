#pragma once
#include <algorithm>
#include <array>
#include "MainQuest.h"

namespace QuestTwo
{
enum class Step
{
    Reaction,
    Investigate,
    Shadows,
    Gym,
    Elite,
    Fragment,
    Report,
    Complete
};

using Objective = MainQuest::Objective;

inline const std::array<Objective, 8>& Objectives()
{
    static const std::array<Objective, 8> data = {{
        {"MQ002_REACTION", "루미와 검은 조각의 반응을 확인하세요. [E]", MainQuest::Event::Interact,
         3, 8, "루미",
         "창밖의 그림자가 학교 안으로 들어갔어. 사물함, 도서관, 방송실을 조사해 보자. 출석보다 급한 일이 생겼네!"},
        {"MQ002_INVESTIGATE", "학교 내부의 조사 지점 3곳을 확인하세요. [E]",
         MainQuest::Event::Interact, 6, 4, "나",
         "같은 검은 흔적이다. 세 흔적이 모두 체육관 쪽으로 이어져!"},
        {"MQ002_SHADOWS", "복도에 나타난 그림자 3마리를 처치하세요. [Q / 1]",
         MainQuest::Event::Defeat, 10, 8, "루미", "학생들이 오기 전에 막아야 해. 별빛으로 변신!"},
        {"MQ002_GYM", "체육관 입구를 조사하세요. [E]", MainQuest::Event::Interact, 21, 8, "나",
         "체육관 문 안쪽에서 큰 반응이 느껴져. 체육 수행평가보다 무서운 건 처음이야."},
        {"MQ002_ELITE", "강화 그림자를 처치하세요. 붉은 공격 예고를 피하세요.",
         MainQuest::Event::Defeat, 15, 8, "루미",
         "조각의 힘을 먹고 커진 그림자야. 돌진과 내려찍기를 조심해!"},
        {"MQ002_FRAGMENT", "체육관 중앙의 검은 조각을 획득하세요. [E]", MainQuest::Event::Interact,
         15, 8, "나", "검은 조각 안에 작은 빛이 남아 있어. 학교를 뒤덮던 기운도 사라졌어."},
        {"MQ002_REPORT", "루미에게 조사 결과를 보고하세요. [E]", MainQuest::Event::Interact, 3, 8,
         "루미",
         "이 힘이면 봉인을 정화할 수 있어! 조각을 더 알아보려면 미아를 찾아야 해. 먼저 체육관 창고의 봉인에 2번 마법을 써 봐."},
        {"MQ002_COMPLETE", "완료 · [2] 별빛 정화로 창고 봉인을 해제하고 보상을 찾으세요.",
         MainQuest::Event::Interact, 20, 5, "루미",
         "다음 이야기: 미아를 찾아서. 학교 사건은 해결됐어!"},
    }};
    return data;
}

struct Investigation
{
    float x, y;
    const char* name;
    const char* text;
};

inline const std::array<Investigation, 3>& Investigations()
{
    static const std::array<Investigation, 3> data = {{
        {6, 4, "사물함", "사물함에 차가운 검은 손자국이 있어. 주인은 오늘 결석인가…?"},
        {12, 4, "도서관", "책장이 저절로 넘어가고 있어. 체육관 대출 기록에 검은 가루가 묻어 있다."},
        {18, 4, "방송실", "방송에서 체육관 쪽 발소리가 들려. 시험 방송이라고 하기엔 너무 섬뜩해."},
    }};
    return data;
}

struct StoryState
{
    Step step = Step::Reaction;
    unsigned clues = 0;
    bool sealOpened = false;
    bool treasureTaken = false;
    int fragments = 4;
    Rpg::Progress progress;

    const Objective& Current() const
    {
        return Objectives()[static_cast<int>(step)];
    }

    bool Gym() const
    {
        return step >= Step::Elite;
    }

    bool Fighting() const
    {
        return step == Step::Shadows || step == Step::Elite;
    }

    int ClueCount() const
    {
        return int(clues & 1) + int((clues >> 1) & 1) + int((clues >> 2) & 1);
    }

    bool Inspect(unsigned index)
    {
        if (step != Step::Investigate || index >= 3 || (clues & (1u << index)))
        {
            return false;
        }
        clues |= 1u << index;
        if (clues == 7)
        {
            step = Step::Shadows;
        }
        return true;
    }

    bool Advance(MainQuest::Event event)
    {
        if (step == Step::Complete || step == Step::Investigate || Current().event != event)
        {
            return false;
        }
        if (step == Step::Fragment)
        {
            ++fragments;
        }
        if (step == Step::Report)
        {
            progress.Award(180);
            progress.coins += 150;
            progress.crystals += 1;
        }
        step = static_cast<Step>(static_cast<int>(step) + 1);
        return true;
    }

    bool OpenSeal()
    {
        if (step != Step::Complete || sealOpened)
        {
            return false;
        }
        sealOpened = true;
        return true;
    }

    bool TakeTreasure()
    {
        if (!sealOpened || treasureTaken)
        {
            return false;
        }
        treasureTaken = true;
        progress.crystals += 2;
        progress.manaPotions = (std::min)(99, progress.manaPotions + 2);
        return true;
    }
};
} // namespace QuestTwo
