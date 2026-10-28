#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include "RuntimeFiles.h"
#include "Profiler.h"

namespace RuntimeFiles
{
std::wstring ResourcePath(const wchar_t* relative)
{
    wchar_t executable[32768] = {};
    const DWORD length = GetModuleFileNameW(nullptr, executable, 32768);
    if (length == 0 || length >= 32768)
    {
        return {};
    }
    std::wstring directory(executable);
    directory = directory.substr(0, directory.find_last_of(L"\\/"));
    for (int depth = 0; depth < 6; ++depth)
    {
        const auto candidate = directory + L"\\resource\\" + relative;
        const DWORD attributes = GetFileAttributesW(candidate.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            return candidate;
        }
        const auto separator = directory.find_last_of(L"\\/");
        if (separator == std::wstring::npos)
        {
            break;
        }
        directory.resize(separator);
    }
    return {};
}

std::wstring Directory()
{
    wchar_t base[32768] = {};
    DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", base, 32768);
    if (count == 0 || count >= 32768)
    {
        return {};
    }

    std::wstring directory = std::wstring(base) + L"\\Emberwick";
    if (!CreateDirectoryW(directory.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return {};
    }
    return directory;
}

std::wstring Path(const wchar_t* filename)
{
    std::wstring directory = Directory();
    return directory.empty() ? std::wstring() : directory + L"\\" + filename;
}

bool Commit(const std::wstring& temporary, const std::wstring& destination)
{
    Performance::Scope scope("cpu.storage.atomic_commit_ms");
    bool committed = MoveFileExW(temporary.c_str(), destination.c_str(),
                                 MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    Performance::Profiler::Get().Count(committed ? "storage.commits_ok" : "storage.commits_failed");
    return committed;
}
} // namespace RuntimeFiles
