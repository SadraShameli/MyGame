#pragma once
#include <string>

namespace MyGame
{
	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved, WindowMinimize,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum EventCategory
	{
		None = 0,
		EventCategoryApplication = 1 << 0,
		EventCategoryInput = 1 << 1,
		EventCategoryKeyboard = 1 << 2,
		EventCategoryMouse = 1 << 3,
		EventCategoryMouseButton = 1 << 4
	};

	class Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() = 0;
		virtual const char* GetName() = 0;
		virtual int GetCategoryFlags() = 0;
		virtual std::string ToString() { return GetName(); }

		bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
		bool Handled = false;
	};

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		inline bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() override { return category; }
#define EVENT_CLASS_TYPE(type)         static EventType GetStaticType() { return EventType::type; }\
                                       virtual EventType GetEventType() override { return GetStaticType(); }\
                                       virtual const char* GetName() override { return #type; }
}