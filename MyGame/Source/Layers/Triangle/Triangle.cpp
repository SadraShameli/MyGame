#include "CommonHeaders.h"

#include "Triangle.h"

#include "../../Core/Log.h"
#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

#include "../../DirectX/DirectXImpl.h"
#include "../../DirectX/CommandAllocatorPool.h"
#include "../../DirectX/CommandList.h"
#include "../../DirectX/CommandContext.h"
#include "../../DirectX/RootSignature.h"
#include "../../DirectX/PipelineState.h"
#include "../../Renderer/Shader.h"

using namespace DirectX;
using namespace Microsoft::WRL;

struct VertexPosColor
{
	XMFLOAT3 Position;
	XMFLOAT3  Color;
};

static VertexPosColor g_Vertices[8] = {
	{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
	{ XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
	{ XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
	{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
	{ XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
	{ XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
	{ XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
	{ XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD g_Indicies[36] =
{
	0, 1, 2, 0, 2, 3,
	4, 5, 1, 4, 1, 0,
	3, 2, 6, 3, 6, 7,
	1, 5, 6, 1, 6, 2,
	4, 0, 3, 4, 3, 7
};

namespace MyGame
{
	TriangleLayer::TriangleLayer() : Layer("Triangle") {}

	void TriangleLayer::OnAttach()
	{


	}

	void TriangleLayer::OnDetach()
	{


	}

	void TriangleLayer::OnEvent(Event& e)
	{


	}

	void TriangleLayer::OnUpdate()
	{


	}
}