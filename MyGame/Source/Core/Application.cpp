#include "CommonHeaders.h"
#include "Application.h"

#include "Source/Events/AppEvent.h"

#include "Log.h"
#include "Source/Debug/Instrumentor.h"

namespace MyGame {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		MYGAME_PROFILE_FUNCTION();
		MYGAME_ASSERT(s_Instance, "Application already exists!");
		s_Instance = this;

		m_Window = Window::Create(WindowProps());
		m_Window->SetEventCallback(MYGAME_BIND_EVENT_FN(Application::OnEvent));
	}

	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		MYGAME_PROFILE_FUNCTION();

		MYGAME_INFO("{0}", e.ToString());

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(MYGAME_BIND_EVENT_FN(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(MYGAME_BIND_EVENT_FN(Application::OnWindowResize));

		for (auto it = m_LayerStack.end(); it-- != m_LayerStack.begin();)
		{
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}
	}

	void Application::Run() {
		MYGAME_PROFILE_FUNCTION();

		while (m_Running) {

			float time = (float)glfwGetTime();
			Timestep timestep = time - m_LastFrameTime;
			m_LastFrameTime = time;

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate(timestep);

				//m_ImGuiLayer->Begin();
				//{
//					for (Layer* layer : m_LayerStack)
						//layer->OnImGuiRender();
				//}
				//m_ImGuiLayer->End();
			}
			m_Window->OnUpdate();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e) {
		m_Running = false;
		return true;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e) {
		MYGAME_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}

		m_Minimized = false;
		return false;
	}
}

int main()
{
	MyGame::Log::Init();
	MYGAME_INFO("Welcome to MyGame!");

	MyGame::Application* app = new MyGame::Application();
	app->Run();

	delete app;
	return 0;
}
