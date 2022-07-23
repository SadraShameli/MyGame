#pragma once

// Windows
#include "Window.h"

// Layers
#include "LayerStack.h"
#include "Source/Layers/ImGui/ImGuiLayer.h"

#include "Base.h"

namespace MyGame
{
	class Application
	{
	public:
		Application();
		~Application();

		void Run();
		void Close();

		void OnEvent(Event&);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);
		ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get() { return *s_Instance; }
		Window& GetWindow() { return *m_Window; }

	private:
		bool OnWindowClose(WindowCloseEvent&);
		bool OnWindowResize(WindowResizeEvent&);

	private:
		static Application* s_Instance;
		std::unique_ptr<Window> m_Window;

		ImGuiLayer* m_ImGuiLayer;
		LayerStack m_LayerStack;

		float m_LastFrameTime = 0.0f;
		bool m_Running = true;
		bool m_Minimized = false;
	};
}