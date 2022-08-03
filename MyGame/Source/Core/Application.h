#pragma once

// Window
#include "Window.h"

// Layers
#include "LayerStack.h"
#include "../Layers/ImGui/ImGuiLayer.h"

#include "Base.h"

namespace MyGame
{
	class Application
	{
	public:
		void Init();
		void Destroy();

		void Run();
		void Close();

		void OnEvent(Event&);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		Window& GetWindow() { return *m_Window; }
		GLFWwindow* GetNativeWindow() const { return m_Window->GetWindow(); }
		HWND GetWin32Window() const { return glfwGetWin32Window(m_Window->GetWindow()); }

	private:
		bool OnWindowClose(WindowCloseEvent&);
		bool OnWindowResize(WindowResizeEvent&);

	private:
		std::unique_ptr<Window> m_Window;

		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
		bool m_Running = true;
		bool m_Minimized = false;
	};

	extern Application application;
}