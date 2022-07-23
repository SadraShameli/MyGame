#include "CommonHeaders.H"
#include "Window.h"

#include "Source/Core/Assert.h"
#include "Source/Core/Log.h"
#include "Source/Core/Input.h"
#include "Source/Debugs/Instrumentor.h"

#include "Source/Events/AppEvent.h"
#include "Source/Events/MouseEvent.h"
#include "Source/Events/KeyEvent.h"

namespace MyGame {

	static uint8_t s_GLFWWindowCount = 0;
	static void GLFWErrorCallback(int error, const char* description) { MYGAME_ERROR("GLFW Error ({0}): {1}", error, description); }

	Window::Window(const WindowProps& props) { Init(props); }
	Window::~Window() { Shutdown(); }

	std::unique_ptr<Window> Window::Create(WindowProps&& props) { return std::make_unique<Window>(std::forward<WindowProps>(props)); }

	void Window::Init(const WindowProps& props)
	{
		MYGAME_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		MYGAME_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWWindowCount)
		{
			MYGAME_PROFILE_SCOPE("glfwInit");
			MYGAME_INFO("Initializing GLFW");
			MYGAME_ASSERT(glfwInit(), "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		MYGAME_PROFILE_SCOPE("glfwCreateWindow");
#ifdef MYGAME_DEBUG
		//if (Renderer::GetAPI() == RendererAPI::API::OpenGL)
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
		m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
		++s_GLFWWindowCount;

		glfwMakeContextCurrent(m_Window);
		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;

				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event); });

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}});


		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event); });

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, false);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, true);
				data.EventCallback(event);
				break;
			}
			}});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event); });
	}

	void Window::Shutdown() {

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (!s_GLFWWindowCount)
			glfwTerminate();
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_Window);
		//m_Context->SwapBuffers();
	}

	void Window::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool Window::IsVSync() const { return m_Data.VSync; }
}