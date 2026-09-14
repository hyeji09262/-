#include "stdafx.h"
#define NOMINMAX
#include <windows.h>
#include "RuntimeFiles.h"

namespace RuntimeFiles
{
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
    return MoveFileExW(temporary.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
}
} // namespace RuntimeFiles
