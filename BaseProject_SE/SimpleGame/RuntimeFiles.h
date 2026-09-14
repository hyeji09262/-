#pragma once
#include <string>

namespace RuntimeFiles
{
// Windows user-local data, independent of the executable / debugger working directory.
std::wstring Directory();
std::wstring Path(const wchar_t* filename);
bool Commit(const std::wstring& temporary, const std::wstring& destination);
} // namespace RuntimeFiles
