#include <cassert>
#include "../SimpleGame/Transformation.h"

int main()
{
    for (float dt : {.016f, .033f, .05f})
    {
        Transformation::Sequence sequence;
        sequence.Start();
        int completed = 0;
        for (float time = 0; time < 4; time += dt)
        {
            completed += sequence.Update(dt) ? 1 : 0;
        }
        assert(completed == 1 && !sequence.Active());
        sequence.Start();
        assert(sequence.Active());
    }
    const auto start = Transformation::Sample(0, 1, 0);
    const auto middle = Transformation::Sample(1.5f, 1, 0);
    const auto end = Transformation::Sample(3, 1, 0);
    assert(start.model == Hunting::ModelId::StudentIdle);
    assert(middle.model == Hunting::ModelId::PlayerCast);
    assert(middle.lift > 10 && middle.glow > start.glow);
    assert(std::abs(end.x - 1) < .001f && std::abs(end.y) < .001f);
    assert(std::abs(end.lift) < .001f);
}
