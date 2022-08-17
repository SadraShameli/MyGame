#include "CommonHeaders.h"

#include "Window.h"
#include "Application.h"

#include "../Core/Log.h"
#include "../Core/Input.h"

#include "../Events/AppEvent.h"
#include "../Events/MouseEvent.h"
#include "../Events/KeyEvent.h"

#include "../Debugs/DebugHelpers.h"
#include "../Debugs/Instrumentor.h"

#include <imgui.h>
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace MyGame
{
	static LRESULT CALLBACK WindowEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
			return true;

		static bool inMove = false;
		switch (message)
		{
		case WM_MOUSEMOVE:
		{
			MouseMovedEvent event((GET_X_LPARAM(lParam)), (GET_Y_LPARAM(lParam)));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN:
		{
			MouseButtonPressedEvent event(static_cast<UINT8>(wParam));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_LBUTTONUP: case WM_RBUTTONUP:
		{
			MouseButtonReleasedEvent event(static_cast<UINT8>(wParam));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			MouseScrolledEvent event(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_CHAR:
		{
			KeyTypedEvent event(static_cast<UINT8>(wParam));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_KEYDOWN:
		{
			KeyPressedEvent event(static_cast<UINT8>(wParam), false);
			Application::Get().OnEvent(event);
			break;
		}
		case WM_KEYUP:
		{
			KeyReleasedEvent event(static_cast<UINT8>(wParam));
			Application::Get().OnEvent(event);
			break;
		}
		case WM_SIZE:
		{
			if (wParam == SIZE_MINIMIZED)
			{
				WindowMinimizeEvent event(true);
				Application::Get().OnWindowMinimize(event);
			}
			else if (inMove)
			{
				WindowResizeEvent event(LOWORD(lParam), HIWORD(lParam));
				Application::Get().OnWindowResize(event);
			}
			else if (wParam == SIZE_RESTORED)
			{
				WindowMinimizeEvent event(false);
				Application::Get().OnWindowMinimize(event);
			}
			break;
		}
		case WM_ENTERSIZEMOVE:
		{
			inMove = true;
			break;
		}
		case WM_EXITSIZEMOVE:
		{
			inMove = false;
			break;
		}
		case WM_CLOSE:
		{
			Application::Get().OnWindowClose();
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	Window::Window(const WindowProps& props)
	{
		m_Data = props;

		WNDCLASSEX windowClass = {};
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowEvent;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = m_Data.Title.c_str();
		RegisterClassEx(&windowClass);

		m_Handle = CreateWindow(windowClass.lpszClassName, m_Data.Title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			m_Data.Width, m_Data.Height, nullptr, nullptr, nullptr, nullptr);
	}

	void Window::Visibility(bool State) { ShowWindow(m_Handle, State); }

	static MSG msg = {};
	void Window::OnUpdate()
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}