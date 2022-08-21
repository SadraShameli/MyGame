#pragma once

#include "Camera.h"
#include "TextureManager.h"

#include "../Scene/Components.h"
#include "../DirectX/DirectXImpl.h"

#include <DirectXMath.h>

namespace MyGame
{
	class Renderer
	{
	public:
		static void Init();
		static void OnUpdate();
		static void OnWindowResize(int width, int height) { DirectXImpl::OnResize(width, height); }

		static void BeginScene(const BaseCamera& camera, const DirectX::XMMATRIX& transform);
		static void BeginScene(const Camera& camera);

		static void EndScene();
		static void Flush();

		static void DrawQuad(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& size, const DirectX::XMVECTOR& color);
		static void DrawQuad(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& size, const TextureRef& texture,
			float tilingFactor = 1.0f, const DirectX::XMVECTOR& tintColor = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void DrawQuad(const DirectX::XMMATRIX& transform, const DirectX::XMVECTOR& color, int entityID = -1);
		static void DrawQuad(const DirectX::XMMATRIX& transform, const TextureRef& texture, float tilingFactor = 1.0f,
			const DirectX::XMVECTOR& tintColor = { 1.0f, 1.0f, 1.0f, 1.0f }, int entityID = -1);

		static void DrawRotatedQuad(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& size, float rotation, const DirectX::XMVECTOR& color);
		static void DrawRotatedQuad(const DirectX::XMVECTOR& position, const DirectX::XMVECTOR& size, float rotation, const TextureRef& texture,
			float tilingFactor = 1.0f, const DirectX::XMVECTOR& tintColor = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void DrawCube(const DirectX::XMMATRIX& transform, float r, float g, float b, int entityID = -1) { DrawCube(transform, { r, g, b }); };
		static void DrawCube(const DirectX::XMMATRIX& transform, const DirectX::XMVECTOR& color, int entityID = -1);

		static void DrawCircle(const DirectX::XMMATRIX& transform, const DirectX::XMVECTOR& color, float thickness = 1.0f, float fade = 0.005f, int entityID = -1);

		static void DrawSprite(const DirectX::XMMATRIX& transform, SpriteRendererComponent& src, int entityID);

		struct Statistics
		{
			uint32_t DrawCalls = 0;
			uint32_t TriangleCount = 0;
			uint32_t QuadCount = 0;
			uint32_t CubeCount = 0;
			uint32_t CircleCount = 0;

			uint32_t GetTotalVertexCount() { return TriangleCount * 3 + QuadCount * 4 + CubeCount * 8 + CircleCount * 4; }
			uint32_t GetTotalIndexCount() { return QuadCount * 6 + CubeCount * 36 + CircleCount * 6; }
		};

		static void ResetStats();
		static Statistics GetStats();

	private:
		static void StartBatch();
		static void NextBatch();
	};
}
