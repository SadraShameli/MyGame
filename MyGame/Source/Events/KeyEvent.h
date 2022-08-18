#pragma once

#include "../Events/Event.h"
#include "../Events/EventCodes/KeyCodes.h"

namespace MyGame
{
	class KeyEvent : public Event
	{
	public:
		inline int GetKeyCode() { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(const int keycode) : m_KeyCode(keycode) {}
		int m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode) : KeyEvent(keycode) {}

		inline std::string ToString() override { return "KeyPressedEvent: " + std::to_string(m_KeyCode); }

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}

		inline std::string ToString() override { return "KeyReleasedEvent: " + std::to_string(m_KeyCode); }

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(int keycode) : KeyEvent(keycode) {}

		inline std::string ToString() override { return "KeyTypedEvent: " + std::to_string(m_KeyCode); }

		EVENT_CLASS_TYPE(KeyTyped)
	};
}