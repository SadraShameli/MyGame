#pragma once

#include "../Events/Event.h"

// Window API
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <memory>
#include <functional>

namespace MyGame
{
	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "MyGame", int width = 1600, int height = 900) : Title(title), Width(width), Height(height) {}
	};

	class Window
	{
	public:
		Window(const WindowProps&);
		~Window();

		uint32_t GetWidth() const { return m_Data.Width; }
		uint32_t GetHeight() const { return m_Data.Height; }

		static std::unique_ptr<Window> Create(WindowProps&&);

		void OnUpdate();
		void SetEventCallback(const std::function<void(Event&)>& callback) { m_Data.EventCallback = callback; }
		void SetVSync(bool);
		bool IsVSync() const;

		GLFWwindow* GetWindow() const { return m_Window; }

	private:
		void Init(const WindowProps&);
		void Shutdown();

	private:
		GLFWwindow* m_Window;

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;
			bool VSync;

			std::function<void(Event&)> EventCallback;
		};

		WindowData m_Data;
	};
}