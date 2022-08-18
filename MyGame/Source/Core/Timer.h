#pragma once

#include "Log.h"

#include <source_location>
#include <chrono>

namespace MyGame
{
	class Timer
	{
	public:
		Timer(const std::source_location& location = std::source_location::current()) { m_Location = location;  Reset(); }
		//~Timer() { MYGAME_INFO("Function {0} done in {1} milliseconds", m_Location.function_name(), ElapsedMillis()); }

		void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

		int64_t ElapsedSeconds() { return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int64_t ElapsedMillis() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int64_t ElapsedMicros() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int64_t ElapsedNanos() { return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }

		const char* GetFunctionName() { return m_Location.function_name(); }
		const char* GetFileName() { return m_Location.file_name(); }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
		std::source_location m_Location;
	};

	class Timestep
	{
	public:
		Timestep() { m_StartTime = std::chrono::steady_clock::now(); }

		void Update() { m_Time = std::chrono::steady_clock::now(); }
		float GetSeconds() const { return (float)std::chrono::duration_cast<std::chrono::seconds>(m_Time - m_StartTime).count(); }
		float GetMilliseconds() const { return GetSeconds() * 1000; }

		operator float() const { return GetSeconds(); }

	private:
		std::chrono::steady_clock::time_point m_Time;
		std::chrono::steady_clock::time_point m_StartTime;
	};
}