#include "CommonHeaders.h"

#include "Application.h"
#include "Log.h"

#include "../Renderer/Renderer.h"
#include "../Events/AppEvent.h"

#include "../Debugs/DebugHelpers.h"
#include "../Debugs/Instrumentor.h"

namespace MyGame
{
	Application::Application()
	{
		s_Instance = this;

		Log::Init();
		MYGAME_INFO("Welcome to MyGame!");

		m_Window = std::make_unique<Window>(std::forward<WindowProps>(WindowProps()));
		Renderer::Init();

		PushLayer(new TriangleLayer());
		//PushOverlay(new ImGuiLayer());

		m_Window->Visibility(true);
	}

	void Application::Destroy()
	{
		//Renderer::Shutdown();
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer)
	{
		m_LayerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	void Application::OnEvent(Event& e)
	{
		MYGAME_INFO_EVENTS(e);

		for (auto it = m_LayerStack.end(); --it != m_LayerStack.begin();)
		{
			if (e.Handled) break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate();
					layer->OnImGuiRender();
				}
			}

			m_Window->OnUpdate();
		}
	}

	void Application::OnWindowResize(WindowResizeEvent& e) { Renderer::OnWindowResize(e.GetWidth(), e.GetHeight()); }
}

int main()
{
	std::unique_ptr<MyGame::Application> app = std::make_unique<MyGame::Application>();
	app->Run();
	app->Destroy();

	return 0;
}