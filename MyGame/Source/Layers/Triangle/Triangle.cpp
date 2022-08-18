#include "CommonHeaders.h"

#include "Triangle.h"

#include "Core/Log.h"
#include "Debugs/Instrumentor.h"
#include "Debugs/DebugHelpers.h"

#include "DirectX/DirectXImpl.h"
#include "DirectX/CommandContext.h"
#include "DirectX/RootSignature.h"
#include "DirectX/PipelineState.h"
#include "Renderer/Shader.h"
#include "Renderer/Camera.h"

using namespace DirectX;
using namespace Microsoft::WRL;

namespace MyGame
{
	TriangleLayer::TriangleLayer() : Layer("Triangle") {}

	Camera cam2;

	struct VertexPosColor
	{
		XMFLOAT3 position;
		XMFLOAT3 color;
	};

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	RootSignature sig;
	DepthBuffer depth(1.0f);
	GraphicsPSO pso(L"Triangle");

	static VertexPosColor g_Vertices[8] = {
	{ { -1.0f, -1.0f, -1.0f }, {0.0f, 0.0f, 0.0f}}, // 0
	{ {-1.0f,  1.0f, -1.0f}, {0.0f, 1.0f, 0.0f} }, // 1
	{ {1.0f,  1.0f, -1.0f }, {1.0f, 1.0f, 0.0f} }, // 2
	{ {1.0f, -1.0f, -1.0f }, {1.0f, 0.0f, 0.0f} }, // 3
	{ {-1.0f, -1.0f,  1.0f }, {0.0f, 0.0f, 1.0f} }, // 4
	{ {-1.0f,  1.0f,  1.0f }, {0.0f, 1.0f, 1.0f} }, // 5
	{ {1.0f,  1.0f,  1.0f }, {1.0f, 1.0f, 1.0f} }, // 6
	{ {1.0f, -1.0f,  1.0f }, {1.0f, 0.0f, 1.0f} }  // 7
	};

	static WORD g_Indicies[36] =
	{
		0, 1, 2, 0, 2, 3,
		4, 6, 5, 4, 7, 6,
		4, 5, 1, 4, 1, 0,
		3, 2, 11, 3, 6, 7,
		1, 5, 6, 1, 6, 2,
		4, 0, 3, 4, 3, 7
	};

	void TriangleLayer::OnAttach()
	{
		for (auto& rtv : DirectXImpl::RenderTargets)
			rtv.SetClearColor({ 0.0f, 0.2f, 0.4f, 1.0f });

		depth.Create(L"Triangle Depth Buffer", 1600, 900, DXGI_FORMAT_D32_FLOAT);

		D3D12_ROOT_SIGNATURE_FLAGS rootSigFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		sig.Reset(1, 0);
		sig[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, D3D12_SHADER_VISIBILITY_VERTEX);
		sig.Finalize(L"Triangle", rootSigFlags);

		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;
		MYGAME_HRESULT_VALIDATE(Shader::D3CompileVertexShader(&vertexShader, L"Assets/Shaders/ShaderVertex.hlsl"));
		MYGAME_HRESULT_VALIDATE(Shader::D3CompilePixelShader(&pixelShader, L"Assets/Shaders/ShaderPixel.hlsl"));

		D3D12_INPUT_ELEMENT_DESC inputLayoutDesc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		pso.SetRootSignature(sig);
		pso.SetInputLayout(inputLayoutDesc, _countof(inputLayoutDesc));
		pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		pso.SetVertexShader(vertexShader.Get());
		pso.SetPixelShader(pixelShader.Get());
		pso.SetDepthTargetFormat(DXGI_FORMAT_D32_FLOAT);
		pso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM);
		pso.Finalize();

		const UINT vertexBufferSize = sizeof(g_Vertices);

		auto d1 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto d2 = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ThrowIfFailed(DirectXImpl::Device->CreateCommittedResource(&d1, D3D12_HEAP_FLAG_NONE, &d2, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_vertexBuffer)));

		UINT8* pVertexDataBegin = nullptr;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, g_Vertices, sizeof(g_Vertices));
		m_vertexBuffer->Unmap(0, nullptr);

		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;

		const UINT indexBufferSize = sizeof(g_Indicies);

		auto d3 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto d4 = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
		ThrowIfFailed(DirectXImpl::Device->CreateCommittedResource(&d3, D3D12_HEAP_FLAG_NONE, &d4, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

		UINT8* pIndexDataBegin = nullptr;
		CD3DX12_RANGE readRange2(0, 0);
		ThrowIfFailed(m_indexBuffer->Map(0, &readRange2, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, g_Indicies, indexBufferSize);
		m_indexBuffer->Unmap(0, nullptr);

		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		m_indexBufferView.SizeInBytes = indexBufferSize;


		cam2 = Camera(90.0f, 1.778f, 0.1f, 1000.0f);
	}

	void TriangleLayer::OnDetach()
	{


	}

	void TriangleLayer::OnEvent(Event& e)
	{
		cam2.OnEvent(e);
	}

	void TriangleLayer::OnUpdate(Timestep ts)
	{
		cam2.OnUpdate(ts);

		XMMATRIX mvp = cam2.GetViewProjection();

		GraphicsContext& ctx = GraphicsContext::Begin(L"Triangle");

		ctx.TransitionResource(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET);
		ctx.ClearColor(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex]);
		ctx.TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		ctx.ClearDepth(depth);
		ctx.SetRootSignature(sig);
		ctx.SetPipelineState(pso);

		ctx.SetViewportAndScissor(0, 0, 1600, 900);
		ctx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ctx.SetVertexBuffer(0, m_vertexBufferView);
		ctx.SetIndexBuffer(m_indexBufferView);
		ctx.SetRenderTarget(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex].GetRTV());
		ctx.SetConstantArray(0, sizeof(XMMATRIX) / 4, &mvp);
		ctx.DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);

		ctx.TransitionResource(DirectXImpl::RenderTargets[DirectXImpl::FrameIndex], D3D12_RESOURCE_STATE_PRESENT);
		ctx.Finish();
	}
}