#include "CommonHeaders.h"

#include "Renderer.h"
#include "Shader.h"

#include "../DirectX/CommandContext.h"
#include "../Debugs/DebugHelpers.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	static RootSignature s_RootSig;
	static DepthBuffer s_DepthBuffer(1.0f);
	static GraphicsPSO s_PSO(L"Renderer PSO");

	struct VertexPositionColor
	{
		XMVECTOR Position;
		XMVECTOR Color;
	};

	struct CircleVertex
	{
		XMVECTOR WorldPosition;
		XMVECTOR LocalPosition;
		XMVECTOR Color;

		float Thickness;
		float Fade;
		int EntityID;
	};

	struct LineVertex
	{
		XMVECTOR Position;
		XMVECTOR Color;

		int EntityID;
	};

	struct RendererData
	{
		struct QuadType
		{
			static constexpr uint32_t Limit = 256;
			static constexpr uint32_t MaxVertices = Limit * 4;
			static constexpr uint32_t MaxIndices = Limit * 6;

			VertexPositionColor* VertexBegin = new VertexPositionColor[MaxVertices];
			VertexPositionColor* VertexOffset = nullptr;
			uint16_t* Indices = new uint16_t[MaxIndices];
			uint32_t IndexCount = 0;

			XMVECTOR Vertices[4] =
			{
				{ -0.5f, -0.5f, 0.0f, 1.0f },
				{ 0.5f, -0.5f, 0.0f, 1.0f },
				{ 0.5f, 0.5f, 0.0f, 1.0f },
				{ -0.5f, 0.5f, 0.0f, 1.0f }
			};
		} Quad;

		struct CubeType
		{
			static constexpr uint32_t Limit = 256;
			static constexpr uint32_t MaxVertices = Limit * 8;
			static constexpr uint32_t MaxIndices = Limit * 36;

			VertexPositionColor* VertexBegin = new VertexPositionColor[MaxVertices];
			VertexPositionColor* VertexOffset = nullptr;
			uint16_t* Indices = new uint16_t[MaxIndices];
			uint32_t IndexCount = 0;

			XMVECTOR Vertices[8] =
			{
				{ -1.0f, -1.0f, -1.0f },
				{ 1.0f, -1.0f, -1.0f },
				{ 1.0f, 1.0f, -1.0f },
				{ -1.0f, 1.0f, -1.0f },
				{ -1.0f, -1.0f, 1.0f },
				{ 1.0f, -1.0f, 1.0f },
				{ 1.0f, 1.0f, 1.0f },
				{ -1.0f, 1.0f, 1.0f }
			};

		} Cube;

		struct CircleType
		{
			static constexpr uint32_t Limit = 256;
			static constexpr uint32_t MaxVertices = Limit * 4;
			static constexpr uint32_t MaxIndices = Limit * 6;

			CircleVertex* VertexBegin = new CircleVertex[MaxVertices];
			CircleVertex* VertexOffset = nullptr;
			uint16_t* Indices = new uint16_t[MaxIndices];
			uint32_t IndexCount = 0;
		} Circle;

		static constexpr uint32_t MaxTextureSlots = 32;
		std::array<TextureRef, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		struct CameraData
		{
			XMMATRIX ViewProjection;
		} CameraBuffer = {};

		Renderer::Statistics Stats;

	} s_Data;

	void Renderer::Init()
	{
		DirectXImpl::OnInit();

		s_DepthBuffer.Create(L"Renderer Depth Buffer", 1600, 900, DXGI_FORMAT_D32_FLOAT);

		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		s_RootSig.Reset(1, 0);
		s_RootSig[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		s_RootSig.Finalize(L"Renderer Root Signature", rootSigFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		MYGAME_HRESULT_VALIDATE(Shader::D3CompileVertexShader(&vertexShader, L"Assets/Shaders/ShaderVertex.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::D3CompilePixelShader(&pixelShader, L"Assets/Shaders/ShaderPixel.hlsl"));

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		s_PSO.SetRootSignature(s_RootSig);
		s_PSO.SetInputLayout(inputLayoutDesc, _countof(inputLayoutDesc));
		s_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		s_PSO.SetVertexShader(vertexShader.Get());
		s_PSO.SetPixelShader(pixelShader.Get());
		s_PSO.SetDepthTargetFormat(DXGI_FORMAT_D32_FLOAT);
		s_PSO.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		s_PSO.Finalize();

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.Quad.MaxIndices; i += 6)
		{
			s_Data.Quad.Indices[i + 0] = offset + 0;
			s_Data.Quad.Indices[i + 1] = offset + 1;
			s_Data.Quad.Indices[i + 2] = offset + 2;
			s_Data.Quad.Indices[i + 3] = offset + 2;
			s_Data.Quad.Indices[i + 4] = offset + 3;
			s_Data.Quad.Indices[i + 5] = offset + 0;

			offset += 4;
		}

		static uint16_t CubeIndices[36] =
		{
			0, 1, 3, 3, 1, 2,
			1, 5, 2, 2, 5, 6,
			5, 4, 6, 6, 4, 7,
			4, 0, 7, 7, 0, 3,
			3, 2, 7, 7, 2, 6,
			4, 5, 0, 0, 5, 1
		};

		uint16_t* CubeIndicesPtr = s_Data.Cube.Indices;
		for (uint32_t i = 0; i < s_Data.Cube.Limit; ++i)
		{
			memcpy(CubeIndicesPtr, CubeIndices, sizeof(CubeIndices));
			CubeIndicesPtr += 36;
		}

		offset = 0;
		for (uint32_t i = 0; i < s_Data.Cube.MaxIndices; i += 36)
		{
			s_Data.Cube.Indices[i + 0] = offset + 0;
			s_Data.Cube.Indices[i + 1] = offset + 1;
			s_Data.Cube.Indices[i + 2] = offset + 2;
			s_Data.Cube.Indices[i + 3] = offset + 2;
			s_Data.Cube.Indices[i + 4] = offset + 3;
			s_Data.Cube.Indices[i + 5] = offset + 0;

			offset += 4;
		}
	}

	void Renderer::OnUpdate()
	{
		DirectXImpl::Present();
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		s_Data.CameraBuffer.ViewProjection = camera.GetViewProjection();
		StartBatch();
	}

	void Renderer::EndScene()
	{
		Flush();
	}

	void Renderer::StartBatch()
	{
		s_Data.Quad.IndexCount = 0;
		s_Data.Quad.VertexOffset = s_Data.Quad.VertexBegin;

		s_Data.Cube.IndexCount = 0;
		s_Data.Cube.VertexOffset = s_Data.Cube.VertexBegin;

		s_Data.Circle.IndexCount = 0;
		s_Data.Circle.VertexOffset = s_Data.Circle.VertexBegin;

		s_Data.TextureSlotIndex = 1;
	}

	// Temp
	static VertexPositionColor g_Vertices[8] = {
	{ { -1.0f, -1.0f, -1.0f }, {0.0f, 0.0f, 0.0f}}, // 0
	{ {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f} }, // 1
	{ {1.0f,  1.0f, -1.0f }, {1.0f, 1.0f, 0.0f} }, // 2
	{ {1.0f, -1.0f, -1.0f }, {1.0f, 0.0f, 0.0f} }, // 3
	{ {-1.0f, -1.0f,  1.0f }, {0.0f, 0.0f, 1.0f} }, // 4
	{ {-1.0f,  1.0f,  1.0f }, {0.0f, 1.0f, 1.0f} }, // 5
	{ {1.0f,  1.0f,  1.0f }, {1.0f, 1.0f, 1.0f} }, // 6
	{ {1.0f, -1.0f,  1.0f }, {1.0f, 0.0f, 1.0f} }  // 7
	};


	void Renderer::Flush()
	{
		GraphicsContext& ctx = GraphicsContext::Begin(L"Triangle");

		ctx.SetRootSignature(s_RootSig);
		ctx.SetPipelineState(s_PSO);
		ctx.SetViewportAndScissor(0, 0, 1600, 900);
		ctx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		ctx.TransitionResource(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ctx.ClearColor(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex]);
		ctx.SetRenderTarget(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex].GetRTV());

		ctx.TransitionResource(s_DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ctx.ClearDepth(s_DepthBuffer);

		ctx.SetConstantArray(0, sizeof(XMMATRIX) / 4, &s_Data.CameraBuffer.ViewProjection);

		if (s_Data.Quad.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.Quad.VertexOffset - (uint8_t*)s_Data.Quad.VertexBegin);

			ctx.SetDynamicVB(0, dataSize, sizeof(VertexPositionColor), s_Data.Quad.VertexBegin);
			ctx.SetDynamicIB(s_Data.Quad.IndexCount, s_Data.Quad.Indices);
			ctx.DrawIndexed(s_Data.Quad.IndexCount, 0, 0);

			s_Data.Stats.DrawCalls++;
		}


		if (s_Data.Cube.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.Cube.VertexOffset - (uint8_t*)s_Data.Cube.VertexBegin);

			ctx.SetDynamicVB(0, sizeof(g_Vertices), sizeof(VertexPositionColor), g_Vertices);
			//ctx.SetDynamicVB(0, dataSize, sizeof(VertexPositionColor), s_Data.Cube.VertexBegin);
			ctx.SetDynamicIB(s_Data.Cube.IndexCount, s_Data.Cube.Indices);
			ctx.DrawIndexed(s_Data.Cube.IndexCount, 0, 0);

			s_Data.Stats.DrawCalls++;
		}

		if (s_Data.Circle.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.Circle.VertexOffset - (uint8_t*)s_Data.Circle.VertexBegin);

			ctx.SetDynamicVB(0, dataSize, sizeof(CircleVertex), s_Data.Circle.VertexBegin);
			ctx.SetDynamicIB(s_Data.Circle.IndexCount, s_Data.Quad.Indices);
			ctx.DrawIndexed(s_Data.Circle.IndexCount, 0, 0);

			s_Data.Stats.DrawCalls++;
		}

		ctx.TransitionResource(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex], D3D12_RESOURCE_STATE_PRESENT);
		ctx.Finish();
	}

	void Renderer::NextBatch()
	{
		//Flush();
		StartBatch();
	}

	void Renderer::DrawQuad(const XMVECTOR& position, const XMVECTOR& size, const XMVECTOR& color)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixScalingFromVector(size);
		DrawQuad(transform, color);
	}

	void Renderer::DrawQuad(const XMVECTOR& position, const XMVECTOR& size, const TextureRef& texture, float tilingFactor, const XMVECTOR& tintColor)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixScalingFromVector(size);
		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer::DrawQuad(const XMMATRIX& transform, const XMVECTOR& color, int entityID)
	{
		if (s_Data.Quad.IndexCount >= s_Data.Quad.MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 4; ++i)
		{
			s_Data.Quad.VertexOffset->Position = XMVector3Transform(s_Data.Quad.Vertices[i], transform);
			s_Data.Quad.VertexOffset->Color = color;

			s_Data.Quad.VertexOffset++;
		}

		s_Data.Quad.IndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer::DrawQuad(const XMMATRIX& transform, const TextureRef& texture, float tilingFactor, const XMVECTOR& tintColor, int entityID)
	{
		constexpr size_t quadVertexCount = 4;
		constexpr XMVECTOR textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_Data.Quad.IndexCount >= s_Data.Quad.MaxIndices)
			NextBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; ++i)
		{
			//if (s_Data.TextureSlots[i] == texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_Data.TextureSlotIndex >= RendererData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)s_Data.TextureSlotIndex;
			//s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		for (size_t i = 0; i < quadVertexCount; ++i)
		{
			s_Data.Quad.VertexOffset->Position = XMVector3Transform(s_Data.Quad.Vertices[i], transform);
			s_Data.Quad.VertexOffset->Color = tintColor;

			s_Data.Quad.VertexOffset++;
		}

		s_Data.Quad.IndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer::DrawRotatedQuad(const XMVECTOR& position, const XMVECTOR& size, float rotation, const XMVECTOR& color)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(rotation)) *
			XMMatrixScaling(XMVectorGetX(position), XMVectorGetY(position), 1.0f);

		DrawQuad(transform, color);
	}

	void Renderer::DrawRotatedQuad(const XMVECTOR& position, const XMVECTOR& size, float rotation, const TextureRef& texture, float tilingFactor, const XMVECTOR& tintColor)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(rotation)) *
			XMMatrixScaling(XMVectorGetX(position), XMVectorGetY(position), 1.0f);

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer::DrawCube(const XMMATRIX& transform, const XMVECTOR& color, int entityID)
	{
		if (s_Data.Cube.IndexCount >= s_Data.Cube.MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 8; ++i)
		{
			s_Data.Cube.VertexOffset->Position = XMVector3Transform(s_Data.Cube.Vertices[i], transform);
			s_Data.Cube.VertexOffset->Color = color;

			s_Data.Cube.VertexOffset++;
		}

		s_Data.Cube.IndexCount += 36;
		s_Data.Stats.CubeCount++;
	}

	void Renderer::DrawCircle(const XMMATRIX& transform, const XMVECTOR& color, float thickness, float fade, int entityID)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			s_Data.Circle.VertexOffset->WorldPosition = XMVector3Transform(s_Data.Quad.Vertices[i], transform);
			s_Data.Circle.VertexOffset->LocalPosition = s_Data.Quad.Vertices[i] * 2.0f;
			s_Data.Circle.VertexOffset->Color = color;
			s_Data.Circle.VertexOffset->Thickness = thickness;
			s_Data.Circle.VertexOffset->Fade = fade;
			s_Data.Circle.VertexOffset->EntityID = entityID;
			s_Data.Circle.VertexOffset++;
		}

		s_Data.Circle.IndexCount += 6;
		s_Data.Stats.QuadCount++;
	}

	void Renderer::DrawSprite(const XMMATRIX& transform, SpriteRendererComponent& src, int entityID)
	{
		//if (src.Texture)
		//	DrawQuad(transform, src.Texture, src.TilingFactor, src.Color, entityID);
		//else
		DrawQuad(transform, src.Color, entityID);
	}

	void Renderer::ResetStats()
	{
		s_Data.Stats = {};
	}

	Renderer::Statistics Renderer::GetStats()
	{
		return s_Data.Stats;
	}

}
