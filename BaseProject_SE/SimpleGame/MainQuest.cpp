#include "stdafx.h"
#include <fstream>
#include <string>
#include "MainQuest.h"
#include "RuntimeFiles.h"
#include "Profiler.h"

namespace MainQuest
{
bool State::Load()
{
    Performance::Scope scope("cpu.storage.main_quest_load_ms");
    const auto path = RuntimeFiles::Path(L"main_quest_001.txt");
    if (path.empty())
    {
        return false;
    }
    std::ifstream file(path.c_str());
    std::string magic;
    int value = 0, reward = 0;
    State candidate;
    if (!(file >> magic >> value >> candidate.fragments >> candidate.progress.totalExperience >>
          candidate.progress.coins >> reward) ||
        magic != "MAGICAL_QUEST_001_V1" || value < 0 || value > static_cast<int>(Step::Complete) ||
        reward != (value == static_cast<int>(Step::Complete) ? 1 : 0) ||
        candidate.fragments != (reward                                     ? 4
                                : value > static_cast<int>(Step::Fragment) ? 1
                                                                           : 0) ||
        candidate.progress.totalExperience != (reward ? 120 : 0) ||
        candidate.progress.coins != (reward ? 100 : 0))
    {
        return false;
    }
    candidate.active = true;
    candidate.step = static_cast<Step>(value);
    candidate.rewardClaimed = reward != 0;
    candidate.progress.Recalculate();
    *this = candidate;
    return true;
}

bool State::Save() const
{
    Performance::Scope scope("cpu.storage.main_quest_save_ms");
    if (!active)
    {
        return true;
    }
    const auto path = RuntimeFiles::Path(L"main_quest_001.txt");
    if (path.empty())
    {
        return false;
    }
    const auto temporary = path + L".tmp";
    {
        std::ofstream file(temporary.c_str(), std::ios::trunc);
        file << "MAGICAL_QUEST_001_V1\n"
             << static_cast<int>(step) << ' ' << fragments << ' ' << progress.totalExperience << ' '
             << progress.coins << ' ' << rewardClaimed << '\n';
        file.flush();
        if (!file)
        {
            return false;
        }
    }
    return RuntimeFiles::Commit(temporary, path);
}
} // namespace MainQuest
