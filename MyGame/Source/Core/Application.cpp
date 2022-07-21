#include "CommonHeaders.h"
#include "Application.h"
#include "Log.h"

#include "Source/Events/AppEvent.h"

namespace MyGame {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		for (;;);
	}

}

int main()
{
	MyGame::Log::Init();
	MYGAME_ERROR("haha");

	MyGame::Application* app = new MyGame::Application();

	app->Run();
	delete app;

	return 0;
}
