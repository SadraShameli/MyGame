#pragma once

#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include "spdlog/spdlog.h"

namespace MyGame
{
	class Log
	{
	public:
		static void Init();

		inline static std::shared_ptr <spdlog::logger>& GetLogger() { return s_Logger; }

	private:
		static std::shared_ptr<spdlog::logger> s_Logger;
	};
}

#ifdef MYGAME_DEBUG
#define MYGAME_TRACE(...) MyGame::Log::GetLogger()->trace(__VA_ARGS__)
#define MYGAME_INFO(...)  MyGame::Log::GetLogger()->info(__VA_ARGS__)
#define MYGAME_WARN(...)  MyGame::Log::GetLogger()->warn(__VA_ARGS__)
#define MYGAME_ERROR(...) MyGame::Log::GetLogger()->error(__VA_ARGS__)

#else
#define MYGAME_TRACE(...) void()
#define MYGAME_INFO(...) void()
#define MYGAME_WARN(...) void()
#define MYGAME_ERROR(...) void()
#endif