#pragma once
#include <array>
#include "RpgProgress.h"

namespace MainQuest
{
enum class Step
{
    Jisoo,
    Guard,
    Clerk,
    Alley,
    Fragment,
    Bag,
    Yuna,
    Escape,
    Lumi,
    Transform,
    Shadows,
    Boss,
    CheckYuna,
    Report,
    Complete
};

enum class Event
{
    Interact,
    Arrive,
    Timer,
    Defeat
};

struct Objective
{
    const char* id;
    const char* text;
    Event event;
    float x, y;
    const char* speaker;
    const char* dialogue;
};

inline const std::array<Objective, 15>& Objectives()
{
    static const std::array<Objective, 15> data = {{
        {"MQ001_01_TALK_JISOO", "지수에게 유나의 소식을 물어보세요.", Event::Interact, 17, 8,
         "지수", "유나가 연락을 안 받아. 경비 아저씨가 마지막으로 봤대. 같이 찾아줘!"},
        {"MQ001_02_ASK_SCHOOL_GUARD", "학교 경비원과 대화하세요.", Event::Interact, 21, 6, "경비원",
         "유나? 편의점 쪽으로 가던데. 학교 뒤 골목은 위험하니 조심하렴."},
        {"MQ001_03_ASK_STORE_WORKER", "편의점 직원에게 물어보세요.", Event::Interact, 10, 10,
         "편의점 직원", "분홍 리본 학생이죠? 검은 무언가를 따라 뒤편 골목으로 갔어요."},
        {"MQ001_04_ENTER_BACK_ALLEY", "골목 입구에서 [E]로 들어가세요.", Event::Interact, 22, 12,
         "나", "여기가 학교 뒤 골목… 유나, 안에 있어?"},
        {"MQ001_05_FIND_BLACK_FRAGMENT", "빛나는 검은 조각을 조사하세요.", Event::Interact, 5, 8,
         "나", "검은 조각… 차갑지만 안쪽에서 이상한 힘이 느껴져."},
        {"MQ001_06_FIND_YUNA_BAG", "유나의 가방을 조사하세요.", Event::Interact, 10, 8, "나",
         "이 리본은 유나 가방이야. 두고 갈 애가 아닌데!"},
        {"MQ001_07_FIND_YUNA", "골목 끝의 유나를 확인하세요.", Event::Interact, 17, 8, "나",
         "유나! 정신 차려! …저 그림자는 뭐야? 일단 괴물을 유나에게서 떼어내자!"},
        {"MQ001_08_ESCAPE_SHADOWS", "Shift로 달려 골목 입구로 도망치세요!", Event::Arrive, 3, 8,
         "나", "출구가 막혔어! 이제 어떡하지?"},
        {"MQ001_09_MEET_LUMI", "루미에게 다가가 [E]로 대화하세요.", Event::Interact, 3, 7, "루미",
         "그 검은 조각, 어디서 주웠어? …설명은 나중에! 살고 싶으면 내 손을 잡아."},
        {"MQ001_10_FIRST_TRANSFORMATION", "루미의 빛을 받아 첫 변신 중…", Event::Timer, 3, 8,
         "루미", "네 마음의 빛을 믿어. 이제 유나를 구하러 가자!"},
        {"MQ001_11_KILL_SHADOWS", "그림자 3마리를 처치하세요.", Event::Defeat, 9, 8, "루미",
         "마법봉으로 공격해! Q 공격 / X 마나 물약 / Z 체력 물약 / 1 별빛 폭발"},
        {"MQ001_12_KILL_SHADOW_EATER", "그림자 포식자를 처치하세요.", Event::Defeat, 15, 8, "루미",
         "바닥의 붉은 예고를 피해! 공격 뒤에는 잠시 빈틈이 생겨."},
        {"MQ001_13_CHECK_YUNA", "유나의 상태를 확인하세요.", Event::Interact, 17, 8, "유나",
         "괴물? 전혀 기억 안 나… 루미? 네 옆에는 아무도 없는데? 먼저 집에 갈게."},
        {"MQ001_14_TALK_LUMI", "루미에게 돌아가 사건을 정리하세요.", Event::Interact, 5, 8, "루미",
         "유나는 날 못 보는구나. 조각의 정체는 나도 몰라. 내일 학교에서 다시 만나자."},
        {"MQ001_COMPLETE", "연락이 끊긴 친구 완료 · 다음 퀘스트 해금", Event::Interact, 5, 8,
         "퀘스트 완료", "변신·마법 해금 / 검은 조각 3개 / 경험치 120 / 골드 100"},
    }};
    return data;
}

class State
{
  public:
    bool active = false;
    Step step = Step::Jisoo;
    int fragments = 0;
    int defeated = 0;
    bool rewardClaimed = false;
    bool epilogueComplete = false;
    Rpg::Progress progress;

    const Objective& Current() const
    {
        return Objectives()[static_cast<int>(step)];
    }

    bool InAlley() const
    {
        return active && step >= Step::Fragment;
    }

    bool CanFight() const
    {
        return active && step >= Step::Shadows;
    }

    const char* NextQuestId() const
    {
        return rewardClaimed ? "MQ_002_SOMETHING_IN_SCHOOL" : "";
    }

    const char* ActiveQuestId() const
    {
        return !active            ? ""
               : epilogueComplete ? "MQ_002_SOMETHING_IN_SCHOOL"
                                  : "MQ_001_MISSING_FRIEND";
    }

    bool FinishEpilogue()
    {
        if (!active || step != Step::Complete || !rewardClaimed || epilogueComplete)
        {
            return false;
        }
        epilogueComplete = true;
        return true;
    }

    bool Emit(Event event)
    {
        if (!active || step == Step::Complete || Current().event != event)
        {
            return false;
        }
        if (step == Step::Shadows && ++defeated < 3)
        {
            return false;
        }
        if (step == Step::Fragment)
        {
            ++fragments;
        }
        if (step == Step::Report && !rewardClaimed)
        {
            fragments += 3;
            progress.Award(120);
            progress.coins += 100;
            rewardClaimed = true;
        }
        step = static_cast<Step>(static_cast<int>(step) + 1);
        return true;
    }

    bool Load();
    bool Save() const;
};
} // namespace MainQuest
