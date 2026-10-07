#pragma once
#include <array>
#include <cstddef>

namespace Prologue
{
enum class Step
{
    News,
    LeaveHome,
    ReachSchool,
    TalkYuna,
    AfterSchool,
    Complete
};

struct DialogueLine
{
    const char* speaker;
    const char* text;
};

// Content stays separate from movement/rendering; #6 is deliberately not started here.
class Story
{
  public:
    Step step = Step::News;
    bool dialogueOpen = false;
    std::size_t line = 0;
    float transitionTime = 0;

    bool IsHome() const
    {
        return step == Step::News || step == Step::LeaveHome;
    }

    void InteractNews()
    {
        if (step == Step::News && !dialogueOpen)
        {
            dialogueOpen = true;
            line = 0;
        }
    }

    void LeaveHome()
    {
        if (step == Step::LeaveHome && !dialogueOpen)
        {
            step = Step::ReachSchool;
        }
    }

    void ReachSchool()
    {
        if (step == Step::ReachSchool)
        {
            step = Step::TalkYuna;
        }
    }

    void TalkYuna()
    {
        if (step == Step::TalkYuna && !dialogueOpen)
        {
            dialogueOpen = true;
            line = 0;
        }
    }

    DialogueLine CurrentLine() const
    {
        static const std::array<DialogueLine, 2> news = {{
            {"아침 뉴스", "최근 도시에서 학생 실종 신고가 증가하고 있습니다."},
            {"나", "우리 동네 이야기잖아… 일단 학교에 가자."},
        }};
        static const std::array<DialogueLine, 4> yuna = {{
            {"유나", "왔네! 요즘 학교 뒤쪽 골목으로 가지 말래."},
            {"유나", "우리 학교 학생도 거기서 마지막으로 봤대."},
            {"나", "아침 뉴스에서도 봤어. 우리도 조심하자."},
            {"유나", "응. 일단 교실로 가자. 지각까지 하면 오늘 정말 최악이야!"},
        }};
        if (step == Step::News)
        {
            return news[line < news.size() ? line : news.size() - 1];
        }
        return yuna[line < yuna.size() ? line : yuna.size() - 1];
    }

    void AdvanceDialogue()
    {
        if (!dialogueOpen)
        {
            return;
        }
        const std::size_t count = step == Step::News ? 2 : 4;
        if (++line < count)
        {
            return;
        }
        dialogueOpen = false;
        step = step == Step::News ? Step::LeaveHome : Step::AfterSchool;
        transitionTime = 0;
    }

    void Update(float dt)
    {
        if (step == Step::AfterSchool)
        {
            transitionTime += dt;
            if (transitionTime >= 3.0f)
            {
                step = Step::Complete;
            }
        }
    }

    const char* Objective() const
    {
        switch (step)
        {
        case Step::News:
            return "TV 앞으로 가서 [E]로 아침 뉴스를 확인하세요.";
        case Step::LeaveHome:
            return "현관으로 이동해 [E]로 등교하세요.";
        case Step::ReachSchool:
            return "벚꽃길을 따라 별빛 학교 정문으로 가세요.";
        case Step::TalkYuna:
            return "정문 앞 유나에게 다가가 [E]로 대화하세요.";
        case Step::AfterSchool:
            return "수업이 끝나고, 방과 후가 되었습니다.";
        case Step::Complete:
            return "#5 완료 · 방과 후 / 다음 이야기는 아직 시작하지 않습니다.";
        }
        return "";
    }
};
} // namespace Prologue
