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

		PushOverlay(new ImGuiLayer());

		m_Window->Visibility(true);
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

		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			if (e.Handled) break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run()
	{
		while (m_Running)
		{
			m_TimeStep.Update();

			if (!m_Minimized)
			{
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
				{
					layer->OnImGuiRender();
					//layer->OnUpdate(m_TimeStep);
				}
				m_ImGuiLayer->End();
			}

			m_Window->OnUpdate();
			Renderer::OnUpdate();
		}
	}

	void Application::OnWindowResize(WindowResizeEvent& e) { Renderer::OnWindowResize(e.GetWidth(), e.GetHeight()); }
}

int main()
{
	std::unique_ptr<MyGame::Application> app = std::make_unique<MyGame::Application>();
	app->Run();

	return 0;
}