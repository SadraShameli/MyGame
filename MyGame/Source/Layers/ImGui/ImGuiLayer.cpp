#include "CommonHeaders.h"

#include "ImGuiLayer.h"

#include "../../Core/Window.h" 
#include "../../Core/Application.h"

#include "../../Core/Log.h"
#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

#include "../../DirectX/DirectXImpl.h"

// Graphics Framework
#include <GLFW/glfw3.h>

// ImGui
#include <imgui.h>
#include <imgui_tables.cpp>
#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_dx12.cpp>
#include <backends/imgui_impl_dx12.h>

namespace MyGame
{
	ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

	void ImGuiLayer::OnAttach()
	{
		MYGAME_PROFILE_FUNCTION();

		//if (!std::filesystem::exists("imgui.ini") && std::filesystem::exists("imgui_default.ini")) { std::filesystem::copy_file("imgui_default.ini", "imgui.ini"); }

		GLFWwindow* window = application.GetNativeWindow();

		// Setup Dear ImGui UI
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsClassic();
		SetDarkMode();

		// Initialize IO Events
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

		float fontSize = 18.0f;
		io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Light.ttf", fontSize);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Regular.ttf", fontSize);

		// Initialize Graphics API
		MYGAME_ASSERT(ImGui_ImplGlfw_InitForOther(window, true));
		DirectX.InitImGui();
	}

	void ImGuiLayer::OnDetach()
	{
		MYGAME_PROFILE_FUNCTION();

		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplDX12_Shutdown();
		ImGui::DestroyContext();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		if (m_BlockEvents)
		{
			ImGuiIO& io = ImGui::GetIO();
			e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
			e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
		}
	}

	int temp1 = 0;
	bool check1;

	void ImGuiLayer::OnImGuiRender()
	{
		ImGui::GetStyle().FrameRounding = 7.0f;
		ImGui::PushItemWidth(200);

		ImGui::Text("Frametime: %.3f ms\nFramerate: %.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// Ambient Occlusion
		std::array<const char*, 3> ambientOcclusionList = { "Off", "Performance", "Quality" };
		ImGui::Text("Ambient Occlusion");
		ImGui::ListBox(" ", &temp1, ambientOcclusionList.data(), 3);

		ImGui::Text("Anisotropic Filtering");
		ImGui::SliderInt(" ", &temp1, 0, 16);

		// Antialiasing	
		ImGui::Text("Anitialiasing - Transparency");
		ImGui::SliderInt(" ", &temp1, 0, 8);
		ImGui::SameLine();
		ImGui::Checkbox("FXAA", &check1);

		// MSAA
		ImGui::Text("MSAA Quality");
		ImGui::SliderInt(" ", &temp1, 0, 8);
	}

	void ImGuiLayer::Begin()
	{
		MYGAME_PROFILE_FUNCTION();

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		MYGAME_PROFILE_FUNCTION();

		// Rendering ImGui
		ImGui::GetIO().DisplaySize = ImVec2((float)application.GetWindow().GetWidth(), (float)application.GetWindow().GetHeight());

		// Rendering DirectX
		DirectX.RenderImGui();
	}

	void ImGuiLayer::SetDarkMode()
	{
		ImGui::StyleColorsDark();
		auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
	}
}
