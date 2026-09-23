#include "stdafx.h"
#include <algorithm>
#include <fstream>
#include <string>
#include "RpgProgress.h"
#include "RuntimeFiles.h"
#include "Profiler.h"

namespace Rpg
{
int Progress::RequiredExperience(int level)
{
    return 40 + (level - 1) * 20;
}

std::int64_t Progress::LevelStart() const
{
    std::int64_t start = 0;
    for (int level = 1; level < stats.level; ++level)
    {
        start += RequiredExperience(level);
    }
    return start;
}

int Progress::ExperienceInLevel() const
{
    return static_cast<int>(totalExperience - LevelStart());
}

void Progress::Recalculate()
{
    totalExperience = (std::max)(std::int64_t(0), (std::min)(ExperienceLimit, totalExperience));
    stats = {};
    std::int64_t remaining = totalExperience;
    while (stats.level < LevelCap && remaining >= RequiredExperience(stats.level))
    {
        remaining -= RequiredExperience(stats.level);
        ++stats.level;
    }

    int growth = stats.level - 1;
    stats.maxHealth = 60 + growth * 12;
    stats.maxMana = 30 + growth * 5;
    stats.attack = 10 + growth * 3;
    stats.defense = 1 + growth;
}

bool Progress::Award(int amount)
{
    int previous = stats.level;
    totalExperience += (std::max)(0, amount);
    Recalculate();
    return stats.level > previous;
}

bool Progress::Load()
{
    Performance::Scope scope("cpu.storage.progress_load_ms");
    const std::wstring path = RuntimeFiles::Path(L"level1_progress.txt");
    if (path.empty())
    {
        return false;
    }

    std::ifstream file(path.c_str());
    std::string magic;
    Progress candidate;
    if (!(file >> magic >> candidate.totalExperience >> candidate.coins >> candidate.potions >>
          candidate.crystals) ||
        magic != "EMBERWICK_PROGRESS_1" || candidate.totalExperience < 0 ||
        candidate.totalExperience > ExperienceLimit || candidate.coins < 0 ||
        candidate.coins > 1000000 || candidate.potions < 0 || candidate.potions > 99 ||
        candidate.crystals < 0 || candidate.crystals > 1000000)
    {
        return false;
    }

    candidate.Recalculate();
    *this = candidate;
    return true;
}

bool Progress::Save() const
{
    Performance::Scope scope("cpu.storage.progress_save_ms");
    Performance::Profiler::Get().Count("storage.progress_save_requests");
    const std::wstring path = RuntimeFiles::Path(L"level1_progress.txt");
    if (path.empty())
    {
        return false;
    }

    const std::wstring temporary = path + L".tmp";
    {
        std::ofstream file(temporary.c_str(), std::ios::trunc);
        file << "EMBERWICK_PROGRESS_1\n"
             << totalExperience << ' ' << coins << ' ' << potions << ' ' << crystals << '\n';
        file.flush();
        if (!file)
        {
            return false;
        }
    }
    return RuntimeFiles::Commit(temporary, path);
}
} // namespace Rpg
