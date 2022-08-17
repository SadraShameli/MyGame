#pragma once

#include <string>
#include <stringapiset.h>

#define MAX_FILEPATH 260

namespace MyGame
{
	namespace Utility
	{
		static inline std::wstring UTF8ToWideString(const std::string& str) { return std::wstring(str.begin(), str.end()); }
		static inline std::string WideStringToUTF8(const std::wstring& wstr) { return std::string(wstr.begin(), wstr.end()); }
	}
}