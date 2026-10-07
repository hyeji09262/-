// Standalone CPU-only story tests; build separately without the game's precompiled header.
#include <cassert>
#include "../SimpleGame/PrologueStory.h"

int main()
{
    using namespace Prologue;
    Story story;
    assert(story.IsHome() && story.step == Step::News);
    story.LeaveHome();
    story.ReachSchool();
    story.TalkYuna();
    story.AdvanceDialogue();
    assert(story.step == Step::News && !story.dialogueOpen);

    story.InteractNews();
    story.AdvanceDialogue();
    story.InteractNews();
    assert(story.dialogueOpen && story.line == 1);
    story.AdvanceDialogue();
    assert(story.step == Step::LeaveHome && !story.dialogueOpen);
    story.LeaveHome();
    assert(!story.IsHome() && story.step == Step::ReachSchool);
    story.TalkYuna();
    assert(!story.dialogueOpen);
    story.ReachSchool();
    story.TalkYuna();
    for (int i = 0; i < 4; ++i)
    {
        assert(story.CurrentLine().text != nullptr);
        story.AdvanceDialogue();
    }
    assert(story.step == Step::AfterSchool && !story.dialogueOpen);
    story.Update(2);
    assert(story.step == Step::AfterSchool);
    story.Update(1);
    assert(story.step == Step::Complete);
    story.TalkYuna();
    story.InteractNews();
    story.AdvanceDialogue();
    story.Update(10);
    assert(story.step == Step::Complete && !story.dialogueOpen);
}
