#pragma once

#include "Window.h"
#include "LayerStack.h"

#include "../Layers/ImGui/ImGuiLayer.h"
#include "../Layers/Triangle/Triangle.h"

namespace MyGame
{
	class Application
	{
	public:
		Application();
		void Run();
		void Destroy();
		void Close() { m_Running = false; }

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		void OnEvent(Event&);
		void OnWindowResize(WindowResizeEvent& e);
		void OnWindowMinimize(WindowMinimizeEvent e) { m_Minimized = e.GetMinimized(); }
		void OnWindowClose() { m_Running = false; }

		inline static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }
		HWND GetNativeWindow() { return m_Window->GetHandle(); }

	private:
		bool m_Running = true, m_Minimized = true;
		Timestep m_TimeStep;

		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer;
		TriangleLayer* m_TriangleLayer;

		std::unique_ptr<Window> m_Window;
		inline static Application* s_Instance;
	};
}