#pragma once

#include "Source/Events/Event.h"
#include "Source/Events/EventCodes/KeyCodes.h"

namespace MyGame {

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(const KeyCode keycode)
			: m_KeyCode(keycode) {}

		KeyCode m_KeyCode;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(const KeyCode keycode, bool isRepeat = false)
			: KeyEvent(keycode), m_IsRepeat(isRepeat) {}

		std::string ToString() const override { return "KeyPressedEvent: " + std::to_string(m_KeyCode) + " - repeat = " + std::to_string(m_IsRepeat); }

		bool IsRepeat() const { return m_IsRepeat; }

		EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(const KeyCode keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override { return "KeyReleasedEvent: " + std::to_string(m_KeyCode); }

		EVENT_CLASS_TYPE(KeyReleased)
	};

	class KeyTypedEvent : public KeyEvent
	{
	public:
		KeyTypedEvent(const KeyCode keycode)
			: KeyEvent(keycode) {}

		std::string ToString() const override { return "KeyTypedEvent: " + std::to_string(m_KeyCode); }

		EVENT_CLASS_TYPE(KeyTyped)
	};
}