#pragma once

#include <string>

namespace MyGame
{
	namespace Utility
	{
		inline std::wstring UTF8ToWideString(const std::string& str)
		{
			return std::wstring(str.begin(), str.end());
		}

		inline std::string WideStringToUTF8(const std::wstring& wstr)
		{
			return std::string(wstr.begin(), wstr.end());
		}
	}
}