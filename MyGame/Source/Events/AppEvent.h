#pragma once

#include "../Events/Event.h"

namespace MyGame
{
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() { return m_Width; }
		inline unsigned int GetHeight() { return m_Height; }

		inline std::string ToString() override { return  "WindowResizeEvent: " + std::to_string(m_Width) + ", " + std::to_string(m_Height); }

		EVENT_CLASS_TYPE(WindowResize) EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		unsigned int m_Width, m_Height;
	};

	class WindowMinimizeEvent : public Event
	{
	public:
		WindowMinimizeEvent(bool minimized) : m_Minimized(minimized) {}

		inline bool GetMinimized() { return m_Minimized; }

		inline std::string ToString() override { return  "WindowMinimizeEvent: " + std::to_string(m_Minimized); }

		EVENT_CLASS_TYPE(WindowMinimize) EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		bool m_Minimized;
	};
}