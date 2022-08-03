#pragma once

#include <chrono>

namespace MyGame
{
	class Timer
	{
	public:
		Timer() { Reset(); }

		void Reset() { m_Start = std::chrono::high_resolution_clock::now(); }

		int Elapsed() { return (int)std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }
		int ElapsedMillis() { return (int)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_Start).count(); }

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_Start;
	};
}