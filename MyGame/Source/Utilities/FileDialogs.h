#pragma once

#include <string>
#include <optional>

namespace MyGame
{
	class FileDialogs
	{
	public:
		static std::optional<std::string> OpenFile(const char*);
		static std::optional<std::string> SaveFile(const char*);
	};
}
