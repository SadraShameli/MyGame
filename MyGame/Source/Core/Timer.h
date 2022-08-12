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
		~Timer() { MYGAME_INFO("{} took {} ms", m_Location.function_name(), ElapsedMillis()); }

		void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

		int Elapsed() { return (int)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int ElapsedMillis() { return (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int ElapsedMicros() { return (int)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int ElapsedNanos() { return (int)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
		std::source_location m_Location;
	};
}