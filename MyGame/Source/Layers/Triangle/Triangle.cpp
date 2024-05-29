#include "CommonHeaders.h"

#include "Triangle.h"

#include "../../Core/Log.h"
#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

#include "../../DirectX/DirectXImpl.h"
#include "../../DirectX/CommandContext.h"
#include "../../DirectX/RootSignature.h"
#include "../../DirectX/PipelineState.h"

#include "../../Renderer/Shader.h"
#include "../../Renderer/Camera.h"
#include "../../Renderer/Renderer.h"
#include "../../Renderer/TextureManager.h"

#include <imgui.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	TriangleLayer::TriangleLayer() : Layer("Triangle") {}

	static Camera camera(45.0f, 1600.0f, 900.0f, 0.1f, 100.0f);

	ManagedTexture* texture;

	void TriangleLayer::OnAttach()
	{
		texture = TextureManager::FindOrLoadTexture(L"Assets/Textures/Lion.dds", false);
	}

	void TriangleLayer::OnDetach()
	{
	}

	void TriangleLayer::OnImGuiRender()
	{
		ImGui::Begin("Stats");

		auto stats = Renderer::GetStats();
		ImGui::Text("Renderer Stats:");
		ImGui::Text("Cubes: %d", stats.CubeCount);
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		ImGui::End();


		ImGui::Begin("Entity");

		ImGui::SliderFloat("X", &x, -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Y", &y, -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Z", &z, -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Roll", &roll, -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Yaw", &yaw, -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Pitch", &pitch, -25.0f, 25.0f, "%.1f");

		ImGui::End();


		ImGui::Begin("Camera Translation");

		ImGui::SliderFloat("X", &camera.FocalPoint.m128_f32[0], -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Y", &camera.FocalPoint.m128_f32[1], -25.0f, 25.0f, "%.1f");
		ImGui::SliderFloat("Z", &camera.FocalPoint.m128_f32[2], -25.0f, 25.0f, "%.1f");

		ImGui::SliderFloat("FoV", &camera.FOV, 1.0f, 120.0f, "%.1f");
		ImGui::SliderFloat("Roll", &camera.Roll, -180.0f, 180.0f, "%.2f");
		ImGui::SliderFloat("Yaw", &camera.Yaw, -180.0f, 180.0f, "%.2f");
		ImGui::SliderFloat("Pitch", &camera.Pitch, -180.0f, 180.0f, "%.2f");
		ImGui::SliderFloat("Distance", &camera.Distance, 1.0f, 25.0f, "%.1f");

		ImGui::End();


		camera.UpdateProjection();

		Renderer::ResetStats();
	}

	void TriangleLayer::OnEvent(Event& e)
	{
		camera.OnEvent(e);
	}

	void TriangleLayer::OnUpdate(Timestep ts)
	{
		camera.OnUpdate(ts);

		TransformComponent comp;

		Renderer::BeginScene(camera);

		Renderer::DrawCube(comp, { 1.0f, 1.0f, 1.0f }, texture);

		comp.Translation = { 12.0f, 12.0f, 12.0f };
		Renderer::DrawCube(comp, { 1.0f, 1.0f, 1.0f });

		comp.Translation = { 5.0f, 5.0f, 5.0f };
		Renderer::DrawCube(comp, { 1.0f, 1.0f, 1.0f });

		Renderer::EndScene();
	}
}