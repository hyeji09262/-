#pragma once
#include <algorithm>
#include <array>
#include "MainQuest.h"
#include "QuestCombat.h"
#include "SchoolLayout.h"

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

namespace Places
{
constexpr MainQuest::Point GymEntry{16, 22};
constexpr MainQuest::Point Fragment{16, 13};
constexpr MainQuest::Point Lumi{5, 21};
constexpr MainQuest::Point Seal{25, 8};
constexpr MainQuest::Point Treasure{28, 6};
} // namespace Places

inline bool StorageAllows(bool opened, float x, float y)
{
    if ((x > 25 && (std::abs(y - 9.f) < .35f || y < 2.5f)) ||
        (std::abs(x - 25.f) < .35f && y < 6.5f))
    {
        return false;
    }
    return opened || x <= 24.7f || y >= 9.3f;
}

inline const std::array<Objective, 8>& Objectives()
{
    static const std::array<Objective, 8> data = {{
        {"MQ002_REACTION", "창문 밖을 지나가는 그림자를 확인하세요.", MainQuest::Event::Interact,
         SchoolLayout::IntroWindowX, SchoolLayout::IntroWindowY, "루미",
         "어제 골목에서 본 기운이 학교로 들어왔어. 복도 사물함, 도서관 대출대, 방송실 장비를 확인하자. 오늘 출석보다 급한 일이 생겼네!"},
        {"MQ002_INVESTIGATE", "학교 내부의 조사 지점 3곳을 확인하세요. [E]",
         MainQuest::Event::Interact, 6, 4, "나",
         "같은 검은 흔적이다. 세 흔적이 모두 체육관 쪽으로 이어져!"},
        {"MQ002_SHADOWS", "복도의 마법사·마도서·가시 씨앗을 처치하세요. [Q / 1]",
         MainQuest::Event::Defeat, 10, 8, "루미",
         "마법탄과 독안개야! 공격 예고를 보고 Shift로 빠져나가. 학생들이 오기 전에 막자!"},
        {"MQ002_GYM", "체육관에 들어가 입구의 검은 흔적을 조사하세요. [E]",
         MainQuest::Event::Interact, Places::GymEntry.x, Places::GymEntry.y, "나",
         "체육관 문 안쪽에서 큰 반응이 느껴져. 체육 수행평가보다 무서운 건 처음이야."},
        {"MQ002_ELITE", "강화 그림자를 처치하세요. 붉은 공격 예고를 피하세요.",
         MainQuest::Event::Defeat, 15, 8, "루미",
         "조각의 힘을 먹고 커진 그림자야. 돌진과 내려찍기를 조심해!"},
        {"MQ002_FRAGMENT", "체육관 중앙의 검은 조각을 획득하세요. [E]", MainQuest::Event::Interact,
         Places::Fragment.x, Places::Fragment.y, "나",
         "검은 조각 안에 작은 빛이 남아 있어. 학교를 뒤덮던 기운도 사라졌어."},
        {"MQ002_REPORT", "체육관 출구 옆 루미에게 조사 결과를 보고하세요. [E]",
         MainQuest::Event::Interact, Places::Lumi.x, Places::Lumi.y, "루미",
         "이 조각에서 정화의 빛이 태어났어! 이제 2번 마법으로 봉인을 열 수 있어. 창고를 확인하고 마을 쇼핑몰의 미아를 찾아보자. 수업 지각 사유에 괴물 퇴치라고 써도 되려나?"},
        {"MQ002_COMPLETE", "완료 · [2] 별빛 정화로 창고 봉인을 해제하고 보상을 찾으세요.",
         MainQuest::Event::Interact, 20, 5, "루미",
         "학교 사건은 해결됐어! 창고의 보급품을 챙기고 체육관 출구로 나가자. 다음 단서는 마을 쇼핑몰의 미아에게 있어."},
    }};
    return data;
}

struct Investigation
{
    int room;
    float x, y;
    const char* name;
    const char* text;
};

inline const std::array<Investigation, 3>& Investigations()
{
    static const std::array<Investigation, 3> data = {{
        {0, 6, 4, "복도 사물함",
         "사물함 문에 어제와 같은 손자국이 있어. 안쪽에는 체육관 열쇠의 검은 자국이 남았어."},
        {1, 8, 5.5f, "도서관 대출 기록",
         "체육관 창고의 오래된 일지가 대출됐어. 책장 사이에서 살아 있는 마도서가 도망갔어!"},
        {3, 5.5f, 6.5f, "방송실 녹음 장비",
         "아무도 없는 체육관의 발소리가 녹음돼 있어. 자동 방송은 편하지만 괴물 생중계까지는 부탁 안 했는데."},
    }};
    return data;
}

struct StoryState
{
    Step step = Step::Reaction;
    unsigned clues = 0;
    bool sealOpened = false;
    bool treasureTaken = false;
    bool rewardClaimed = false;
    bool shopVisited = false;
    int fragments = 4;
    Rpg::Progress progress;

    const Objective& Current() const
    {
        if (step == Step::Complete)
        {
            static const Objective complete{"MQ002_COMPLETE",
                                            "학교 사건 해결 · [E]로 마을로 돌아가세요.",
                                            MainQuest::Event::Interact,
                                            0,
                                            0,
                                            "루미",
                                            "학교의 악몽이 사라졌어. 이제 안심하고 돌아가자!"};
            return complete;
        }
        return Objectives()[static_cast<int>(step)];
    }

    bool Gym() const
    {
        return step >= Step::Elite;
    }

    bool Fighting() const
    {
        return step == Step::Elite;
    }

    bool ReadyToFight(int room, bool transforming) const
    {
        return !transforming && step == Step::Elite && room == 2;
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
            step = Step::Gym;
        }
        return true;
    }

    bool Advance(MainQuest::Event event)
    {
        if (step == Step::Complete || step == Step::Investigate || Current().event != event)
        {
            return false;
        }
        if (step == Step::Reaction && event == MainQuest::Event::Interact)
        {
            step = Step::Investigate;
            return true;
        }
        if (step == Step::Gym && event == MainQuest::Event::Interact)
        {
            step = Step::Elite;
            return true;
        }
        if (step == Step::Elite && event == MainQuest::Event::Defeat)
        {
            if (!rewardClaimed)
            {
                progress.Award(180);
                progress.coins = (std::min)(1000000, progress.coins + 150);
                progress.potions = (std::min)(99, progress.potions + 2);
                progress.manaPotions = (std::min)(99, progress.manaPotions + 2);
                ++fragments;
                rewardClaimed = true;
            }
            sealOpened = treasureTaken = true;
            step = Step::Complete;
            return true;
        }
        return false;
    }

    void Restart()
    {
        step = Step::Reaction;
        clues = 0;
        sealOpened = treasureTaken = false;
        shopVisited = false;
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
        progress.potions = (std::min)(99, progress.potions + 2);
        progress.manaPotions = (std::min)(99, progress.manaPotions + 2);
        return true;
    }
};
} // namespace QuestTwo
