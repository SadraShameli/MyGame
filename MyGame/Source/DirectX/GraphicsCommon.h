#pragma once

#include "BufferHelper.h"
#include "CommandHelper.h"
#include "PipelineState.h"

#include "../Renderer/Texture.h"
#include "../Renderer/TextureManager.h"

namespace MyGame
{
	class GraphicsCommon
	{
	public:
		static void Init();
		static D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultTexture(TextureManager::DefaultTexture texId) { return DefaultTextures[texId].GetSRV(); }

	public:
		inline static SamplerDesc SamplerLinearWrapDesc;
		inline static SamplerDesc SamplerAnisoWrapDesc;
		inline static SamplerDesc SamplerShadowDesc;
		inline static SamplerDesc SamplerLinearClampDesc;
		inline static SamplerDesc SamplerVolumeWrapDesc;
		inline static SamplerDesc SamplerPointClampDesc;
		inline static SamplerDesc SamplerPointBorderDesc;
		inline static SamplerDesc SamplerLinearBorderDesc;

		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
		inline static D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;

		inline static D3D12_RASTERIZER_DESC RasterizerDefault;
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultCw;
		inline static D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
		inline static D3D12_RASTERIZER_DESC RasterizerTwoSided;
		inline static D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
		inline static D3D12_RASTERIZER_DESC RasterizerShadow;
		inline static D3D12_RASTERIZER_DESC RasterizerShadowCW;
		inline static D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;

		inline static D3D12_BLEND_DESC BlendNoColorWrite;
		inline static D3D12_BLEND_DESC BlendDisable;
		inline static D3D12_BLEND_DESC BlendPreMultiplied;
		inline static D3D12_BLEND_DESC BlendTraditional;
		inline static D3D12_BLEND_DESC BlendAdditive;
		inline static D3D12_BLEND_DESC BlendTraditionalAdditive;

		inline static D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
		inline static D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;

		inline static CommandSignature DispatchIndirectCommandSignature = CommandSignature(1);
		inline static CommandSignature DrawIndirectCommandSignature = CommandSignature(1);

		inline static RootSignature CommonRS;
		inline static ComputePSO GenerateMipsLinearPSO[4] =
		{
			{L"Generate Mips Linear CS"},
			{L"Generate Mips Linear Odd X CS"},
			{L"Generate Mips Linear Odd Y CS"},
			{L"Generate Mips Linear Odd CS"},
		};

		inline static ComputePSO GenerateMipsGammaPSO[4] =
		{
			{ L"Generate Mips Gamma CS" },
			{ L"Generate Mips Gamma Odd X CS" },
			{ L"Generate Mips Gamma Odd Y CS" },
			{ L"Generate Mips Gamma Odd CS" },
		};

		inline static GraphicsPSO DownsampleDepthPSO = GraphicsPSO(L"DownsampleDepth PSO");

		inline static Texture DefaultTextures[TextureManager::NumDefaultTextures];
	};
}