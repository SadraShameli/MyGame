#pragma once

#include "../Events/Event.h"

namespace MyGame
{
	struct WindowProps
	{
		std::wstring Title;
		unsigned int Width;
		unsigned int Height;

		WindowProps(const std::wstring& title = L"MyGame", unsigned int width = 1600, unsigned int height = 900) : Title(title), Width(width), Height(height) {}
	};

	class Window
	{
	public:
		Window(const WindowProps&);

		void OnUpdate();

		HWND GetHandle() { return m_Handle; }
		int GetWidth() { return m_Data.Width; }
		int GetHeight() { return m_Data.Height; }
		void Visibility(bool State);

	private:
		HWND m_Handle;
		WindowProps m_Data;
	};
}