#include "CommonHeaders.h"

#include "Renderer.h"
#include "Shader.h"
#include "TextureManager.h"

#include "../DirectX/CommandContext.h"
#include "../DirectX/DescriptorHeap.h"
#include "../Debugs/DebugHelpers.h"

namespace MyGame
{
	using namespace DirectX;
	using namespace Microsoft::WRL;

	static ColorBuffer s_ColorBuffer;
	static RootSignature s_RootSig;
	static DepthBuffer s_DepthBuffer(1.0f);
	static GraphicsPSO s_PSO(L"Renderer");

	static DescriptorHeap s_TexturesHeap;
	static DescriptorHandle s_Textures;

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
		std::array<Texture*, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1;

		struct CameraData
		{
			XMMATRIX ViewProjection;
		} CameraBuffer = {};

		Renderer::Statistics Stats;

	} s_RendererData;

	void Renderer::Init()
	{
		DirectXImpl::Init();

		s_ColorBuffer.Create(L"Renderer Color Buffer", 1600, 900, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
		s_DepthBuffer.Create(L"Renderer Depth Buffer", 1600, 900, DXGI_FORMAT_D32_FLOAT);
		s_TexturesHeap.Create(L"Renderer Texture Heap", D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
		s_Textures = s_TexturesHeap.Allocate(8);

		SamplerDesc DefaultSamplerDesc{};
		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		s_RootSig.Reset(3, 1);
		s_RootSig.InitStaticSampler(0, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
		s_RootSig[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		s_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL);
		s_RootSig[2].InitAsDescriptorTable(0);
		s_RootSig.Finalize(L"Renderer Root Signature", rootSigFlags);

		ComPtr<ID3DBlob> vertexShader, pixelShader;
		MYGAME_HRESULT_VALIDATE(Shader::D3CompileVertexShader(&vertexShader, L"Assets/Shaders/TextureShader.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::D3CompilePixelShader(&pixelShader, L"Assets/Shaders/TextureShader.hlsl"));

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		D3D12_RASTERIZER_DESC RasterizerDefault{};
		RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
		RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
		RasterizerDefault.FrontCounterClockwise = TRUE;
		RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		RasterizerDefault.DepthClipEnable = TRUE;
		RasterizerDefault.MultisampleEnable = FALSE;
		RasterizerDefault.AntialiasedLineEnable = FALSE;
		RasterizerDefault.ForcedSampleCount = 0;
		RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		D3D12_BLEND_DESC BlendDisable{};
		BlendDisable.IndependentBlendEnable = FALSE;
		BlendDisable.RenderTarget[0].BlendEnable = FALSE;
		BlendDisable.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		BlendDisable.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		BlendDisable.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		BlendDisable.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		BlendDisable.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		BlendDisable.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		BlendDisable.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite{};
		DepthStateReadWrite.DepthEnable = TRUE;
		DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		DepthStateReadWrite.StencilEnable = FALSE;
		DepthStateReadWrite.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		DepthStateReadWrite.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		DepthStateReadWrite.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		DepthStateReadWrite.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		DepthStateReadWrite.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateReadWrite.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		DepthStateReadWrite.BackFace = DepthStateReadWrite.FrontFace;

		s_PSO.SetRootSignature(s_RootSig);
		s_PSO.SetRasterizerState(RasterizerDefault);
		s_PSO.SetBlendState(BlendDisable);
		s_PSO.SetDepthStencilState(DepthStateReadWrite);
		s_PSO.SetInputLayout(inputLayoutDesc, _countof(inputLayoutDesc));
		s_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		s_PSO.SetVertexShader(vertexShader.Get());
		s_PSO.SetPixelShader(pixelShader.Get());
		s_PSO.SetDepthTargetFormat(DXGI_FORMAT_D32_FLOAT);
		s_PSO.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		s_PSO.Finalize();

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_RendererData.Quad.MaxIndices; i += 6)
		{
			s_RendererData.Quad.Indices[i + 0] = offset + 0;
			s_RendererData.Quad.Indices[i + 1] = offset + 1;
			s_RendererData.Quad.Indices[i + 2] = offset + 2;
			s_RendererData.Quad.Indices[i + 3] = offset + 2;
			s_RendererData.Quad.Indices[i + 4] = offset + 3;
			s_RendererData.Quad.Indices[i + 5] = offset + 0;

			offset += 4;
		}

		uint16_t CubeIndices[36] =
		{
			0, 1, 3, 3, 1, 2,
			1, 5, 2, 2, 5, 6,
			5, 4, 6, 6, 4, 7,
			4, 0, 7, 7, 0, 3,
			3, 2, 7, 7, 2, 6,
			4, 5, 0, 0, 5, 1
		};

		uint16_t* CubeIndicesPtr = s_RendererData.Cube.Indices;
		for (uint32_t i = 0; i < s_RendererData.Cube.Limit; ++i)
		{
			memcpy(CubeIndicesPtr, CubeIndices, sizeof(CubeIndices));

			for (int i = 0; i < _countof(CubeIndices); ++i)
				CubeIndices[i] += 8;

			CubeIndicesPtr += 36;
		}
	}

	void Renderer::OnUpdate()
	{
		DirectXImpl::Present();
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		s_RendererData.CameraBuffer.ViewProjection = camera.GetViewProjection();
		StartBatch();
	}

	void Renderer::EndScene()
	{
		Flush();
	}

	void Renderer::StartBatch()
	{
		s_RendererData.Quad.IndexCount = 0;
		s_RendererData.Quad.VertexOffset = s_RendererData.Quad.VertexBegin;

		s_RendererData.Cube.IndexCount = 0;
		s_RendererData.Cube.VertexOffset = s_RendererData.Cube.VertexBegin;

		s_RendererData.Circle.IndexCount = 0;
		s_RendererData.Circle.VertexOffset = s_RendererData.Circle.VertexBegin;

		s_RendererData.TextureSlotIndex = 0;
	}

	void Renderer::Flush()
	{
		GraphicsContext& ctx = GraphicsContext::Begin(L"Renderer GraphicsContext");

		ctx.SetRootSignature(s_RootSig);
		ctx.SetPipelineState(s_PSO);
		ctx.SetViewportAndScissor(0, 0, s_ColorBuffer.GetWidth(), s_ColorBuffer.GetHeight());
		ctx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.SetConstantArray(0, sizeof(XMMATRIX) / 4, &s_RendererData.CameraBuffer.ViewProjection);

		ctx.TransitionResource(s_ColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		ctx.ClearColor(s_ColorBuffer);
		ctx.SetRenderTarget(s_ColorBuffer.GetRTV());

		ctx.TransitionResource(s_DepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ctx.ClearDepth(s_DepthBuffer);

		if (s_RendererData.Quad.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.Quad.VertexOffset - (uint8_t*)s_RendererData.Quad.VertexBegin);

			ctx.SetDynamicVB(0, dataSize, sizeof(VertexPositionColor), s_RendererData.Quad.VertexBegin);
			ctx.SetDynamicIB(s_RendererData.Quad.IndexCount, s_RendererData.Quad.Indices);
			ctx.DrawIndexed(s_RendererData.Quad.IndexCount, 0, 0);

			s_RendererData.Stats.DrawCalls++;
		}

		if (s_RendererData.Cube.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.Cube.VertexOffset - (uint8_t*)s_RendererData.Cube.VertexBegin);

			ctx.SetDynamicVB(0, dataSize, sizeof(VertexPositionColor), s_RendererData.Cube.VertexBegin);
			ctx.SetDynamicIB(s_RendererData.Cube.IndexCount, s_RendererData.Cube.Indices);

			ctx.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, s_TexturesHeap.GetHeapPointer());
			ctx.SetDescriptorTable(1, s_Textures);
			ctx.DrawIndexed(s_RendererData.Cube.IndexCount, 0, 0);

			s_RendererData.Stats.DrawCalls++;
		}

		if (s_RendererData.Circle.IndexCount)
		{
			uint32_t dataSize = (uint32_t)((uint8_t*)s_RendererData.Circle.VertexOffset - (uint8_t*)s_RendererData.Circle.VertexBegin);

			ctx.SetDynamicVB(0, dataSize, sizeof(CircleVertex), s_RendererData.Circle.VertexBegin);
			ctx.SetDynamicIB(s_RendererData.Circle.IndexCount, s_RendererData.Quad.Indices);
			ctx.DrawIndexed(s_RendererData.Circle.IndexCount, 0, 0);

			s_RendererData.Stats.DrawCalls++;
		}

		ctx.Finish();
	}

	void Renderer::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void Renderer::DrawQuad(const XMVECTOR& position, const XMVECTOR& size, const XMVECTOR& color)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixScalingFromVector(size);
		DrawQuad(transform, color);
	}

	void Renderer::DrawQuad(const XMVECTOR& position, const XMVECTOR& size, const Texture& texture, float tilingFactor, const XMVECTOR& tintColor)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixScalingFromVector(size);
		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer::DrawQuad(const XMMATRIX& transform, const XMVECTOR& color, int entityID)
	{
		if (s_RendererData.Quad.IndexCount >= s_RendererData.Quad.MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 4; ++i)
		{
			s_RendererData.Quad.VertexOffset->Position = XMVector3Transform(s_RendererData.Quad.Vertices[i], transform);
			s_RendererData.Quad.VertexOffset->Color = color;

			s_RendererData.Quad.VertexOffset++;
		}

		s_RendererData.Quad.IndexCount += 6;
		s_RendererData.Stats.QuadCount++;
	}

	void Renderer::DrawQuad(const XMMATRIX& transform, const Texture& texture, float tilingFactor, const XMVECTOR& tintColor, int entityID)
	{
		constexpr XMVECTOR textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_RendererData.Quad.IndexCount >= s_RendererData.Quad.MaxIndices)
			NextBatch();

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_RendererData.TextureSlotIndex; ++i)
		{
			//if (s_RendererData.TextureSlots[i] == texture)
			{
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.0f)
		{
			if (s_RendererData.TextureSlotIndex >= RendererData::MaxTextureSlots)
				NextBatch();

			textureIndex = (float)s_RendererData.TextureSlotIndex;
			//s_RendererData.TextureSlots[s_RendererData.TextureSlotIndex] = texture;
			s_RendererData.TextureSlotIndex++;
		}

		for (size_t i = 0; i < 4; ++i)
		{
			s_RendererData.Quad.VertexOffset->Position = XMVector3Transform(s_RendererData.Quad.Vertices[i], transform);
			s_RendererData.Quad.VertexOffset->Color = tintColor;

			s_RendererData.Quad.VertexOffset++;
		}

		s_RendererData.Quad.IndexCount += 6;
		s_RendererData.Stats.QuadCount++;
	}

	void Renderer::DrawRotatedQuad(const XMVECTOR& position, const XMVECTOR& size, float rotation, const XMVECTOR& color)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(rotation)) *
			XMMatrixScaling(XMVectorGetX(position), XMVectorGetY(position), 1.0f);

		DrawQuad(transform, color);
	}

	void Renderer::DrawRotatedQuad(const XMVECTOR& position, const XMVECTOR& size, float rotation, const Texture& texture, float tilingFactor, const XMVECTOR& tintColor)
	{
		XMMATRIX transform = XMMatrixTranslationFromVector(position) * XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, XMConvertToRadians(rotation)) *
			XMMatrixScaling(XMVectorGetX(position), XMVectorGetY(position), 1.0f);

		DrawQuad(transform, texture, tilingFactor, tintColor);
	}

	void Renderer::DrawCube(const XMMATRIX& transform, const XMVECTOR& color, int entityID)
	{
		if (s_RendererData.Cube.IndexCount >= s_RendererData.Cube.MaxIndices)
			NextBatch();

		for (size_t i = 0; i < 8; ++i)
		{
			s_RendererData.Cube.VertexOffset->Position = XMVector3Transform(s_RendererData.Cube.Vertices[i], transform);
			s_RendererData.Cube.VertexOffset->Color = color;

			s_RendererData.Cube.VertexOffset++;
		}

		s_RendererData.Cube.IndexCount += 36;
		s_RendererData.Stats.CubeCount++;
	}

	void Renderer::DrawCube(const DirectX::XMMATRIX& transform, const DirectX::XMVECTOR& color, Texture* texture, float tilingFactor, const DirectX::XMVECTOR& tintColor, int entityID)
	{
		constexpr XMVECTOR textureCoords[] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } };

		if (s_RendererData.Quad.IndexCount >= s_RendererData.Quad.MaxIndices)
			NextBatch();

		uint32_t textureIndex = 0;
		for (uint32_t i = 1; i < s_RendererData.TextureSlotIndex; ++i)
		{
			if (s_RendererData.TextureSlots[i] == texture)
			{
				textureIndex = i;
				break;
			}
		}

		if (textureIndex == 0)
		{
			if (s_RendererData.TextureSlotIndex >= RendererData::MaxTextureSlots)
				NextBatch();

			s_RendererData.TextureSlots[s_RendererData.TextureSlotIndex] = texture;
			s_RendererData.TextureSlotIndex++;

			uint32_t DestCount = 1;
			uint32_t SourceCounts[] = { 1 };
			D3D12_CPU_DESCRIPTOR_HANDLE SourceTextures[] =
			{
				texture->GetSRV(),
			};
			DirectXImpl::Device->CopyDescriptors(1, &s_Textures, &DestCount,
				DestCount, SourceTextures, SourceCounts, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		for (size_t i = 0; i < 8; ++i)
		{
			s_RendererData.Cube.VertexOffset->Position = XMVector3Transform(s_RendererData.Cube.Vertices[i], transform);
			s_RendererData.Cube.VertexOffset->Color = color;

			s_RendererData.Cube.VertexOffset++;
		}

		s_RendererData.Cube.IndexCount += 36;
		s_RendererData.Stats.CubeCount++;
	}


	void Renderer::DrawCircle(const XMMATRIX& transform, const XMVECTOR& color, float thickness, float fade, int entityID)
	{
		for (size_t i = 0; i < 4; ++i)
		{
			s_RendererData.Circle.VertexOffset->WorldPosition = XMVector3Transform(s_RendererData.Quad.Vertices[i], transform);
			s_RendererData.Circle.VertexOffset->LocalPosition = s_RendererData.Quad.Vertices[i] * 2.0f;
			s_RendererData.Circle.VertexOffset->Color = color;
			s_RendererData.Circle.VertexOffset->Thickness = thickness;
			s_RendererData.Circle.VertexOffset->Fade = fade;
			s_RendererData.Circle.VertexOffset->EntityID = entityID;
			s_RendererData.Circle.VertexOffset++;
		}

		s_RendererData.Circle.IndexCount += 6;
		s_RendererData.Stats.QuadCount++;
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
		s_RendererData.Stats = {};
	}

	const Renderer::Statistics& Renderer::GetStats()
	{
		return s_RendererData.Stats;
	}

}
