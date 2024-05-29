#include "CommonHeaders.h"

#include "ImGuiLayer.h"

#include "../../Core/Application.h"
#include "../../Core/Log.h"
#include "../../Renderer/Renderer.h"
#include "../../DirectX/CommandContext.h"

#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

#include <imgui.h>
#include <imgui_tables.cpp>
#include <imgui_impl_win32.h>
#include <imgui_impl_win32.cpp>
#include <imgui_impl_dx12.h>
#include <imgui_impl_dx12.cpp>

namespace MyGame
{
	static ColorBuffer s_ColorBuffer;
	extern ColorBuffer s_SceneBuffer;
	static DescriptorHeap s_SrvHeap;

	ImGuiLayer::ImGuiLayer() : Layer("ImGui Layer") {}

	void ImGuiLayer::OnAttach()
	{
		s_SrvHeap.Create(L"ImGui SRV Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		s_ColorBuffer.Create(L"ImGui Color Buffer", 1600, 900, 1, DXGI_FORMAT_R8G8B8A8_UNORM);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		ImGui::StyleColorsClassic();
		SetDarkMode();

		float fontSize = 18.0f;
		io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Light.ttf", fontSize);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Assets/Fonts/OpenSans/OpenSans-Regular.ttf", fontSize);

		ImGui_ImplWin32_Init(Application::Get().GetNativeWindow());
		MYGAME_ASSERT(ImGui_ImplDX12_Init(DirectXImpl::Device, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
			s_SrvHeap.GetHeapPointer(), s_SrvHeap.GetHeapPointer()->GetCPUDescriptorHandleForHeapStart(),
			s_SrvHeap.GetHeapPointer()->GetGPUDescriptorHandleForHeapStart()));
	}

	int temp1 = 0;
	bool check1;

	void ImGuiLayer::Begin()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::OnImGuiRender()
	{
		ImGui::Begin("MyGame");

		ImGui::GetStyle().FrameRounding = 7.0f;
		ImGui::PushItemWidth(200);

		ImGui::Text("Framerate: %.1f FPS\nFrametime: %.3f ms", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		const char* ambientOcclusionList[] = { "Off", "Performance", "Quality" };
		ImGui::Text("Ambient Occlusion");
		ImGui::ListBox(" ", &temp1, ambientOcclusionList, 3);

		ImGui::Text("Anisotropic Filtering");
		ImGui::SliderInt(" ", &temp1, 0, 16);

		ImGui::Text("Anitialiasing - Transparency");
		ImGui::SliderInt(" ", &temp1, 0, 8);
		ImGui::SameLine();
		ImGui::Checkbox("FXAA", &check1);

		ImGui::Text("MSAA Quality");
		ImGui::SliderInt(" ", &temp1, 0, 8);

		ImGui::End();
	}

	void ImGuiLayer::End()
	{
		ImGui::Render();
		Render();
	}

	void ImGuiLayer::Render()
	{
		GraphicsContext& ctx = GraphicsContext::Begin(L"ImGui Layer");

		ctx.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, s_SrvHeap.GetHeapPointer());
		ctx.SetViewportAndScissor(0, 0, s_SceneBuffer.GetWidth(), s_SceneBuffer.GetHeight());
		ctx.TransitionResource(s_SceneBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		ctx.ClearColor(s_SceneBuffer);
		ctx.SetRenderTarget(s_SceneBuffer.GetRTV());

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), ctx.GetCommandList());

		ctx.Finish();
	}

	void ImGuiLayer::OnEvent(Event& e)
	{
		ImGuiIO& io = ImGui::GetIO();
		e.Handled |= e.IsInCategory(EventCategoryMouse) & io.WantCaptureMouse;
		e.Handled |= e.IsInCategory(EventCategoryKeyboard) & io.WantCaptureKeyboard;
	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		s_SrvHeap.Destroy();
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
