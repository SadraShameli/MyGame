#pragma once

#include "../Events/Event.h"
#include "../Events/EventCodes/MouseCodes.h"

namespace MyGame
{
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(int x, int y) : m_MouseX(x), m_MouseY(y) {}

		inline int GetX() { return m_MouseX; }
		inline int GetY() { return m_MouseY; }

		inline std::string ToString() override { return "MouseMovedEvent: " + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY); }

		EVENT_CLASS_TYPE(MouseMoved) EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		int m_MouseX, m_MouseY;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(int offset) : m_Offset(offset) {}

		inline int GetOffset() { return m_Offset; }

		inline std::string ToString()  override { return "MouseScrolledEvent: " + std::to_string(GetOffset()); }

		EVENT_CLASS_TYPE(MouseScrolled)	EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		int m_Offset;
	};

	class MouseButtonEvent : public Event
	{
	public:
		inline int GetMouseButton() { return m_Button; }

		EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)

	protected:
		MouseButtonEvent(int button) : m_Button(button) {}
		int m_Button;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

		inline std::string ToString() override { return "MouseButtonPressedEvent: " + std::to_string(m_Button); }

		EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

		inline std::string ToString() override { return "MouseButtonReleasedEvent: " + std::to_string(m_Button); }

		EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}