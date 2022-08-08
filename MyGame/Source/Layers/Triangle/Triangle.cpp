#include "CommonHeaders.h"

#include "Triangle.h"

#include "../../Core/Window.h" 
#include "../../Core/Application.h"

#include "../../Core/Log.h"
#include "../../Debugs/Instrumentor.h"
#include "../../Debugs/DebugHelpers.h"

namespace MyGame
{
	TriangleLayer::TriangleLayer() : Layer("Triangle") {}

	void TriangleLayer::OnAttach()
	{
		MYGAME_PROFILE_FUNCTION();


	}

	void TriangleLayer::OnDetach()
	{
		MYGAME_PROFILE_FUNCTION();

	}

	void TriangleLayer::OnEvent(Event& e)
	{
		MYGAME_PROFILE_FUNCTION();

	}

	void TriangleLayer::OnUpdate(Timestep time)
	{
		//struct VertexPosColor
		//{
		//	glm::vec3 Position;
		//	glm::vec3  Color;
		//};

		//static VertexPosColor g_Vertices[8] = {
		//	{ glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f) }, // 0
		//	{ glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) }, // 1
		//	{ glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 0.0f) }, // 2
		//	{ glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f) }, // 3
		//	{ glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.0f, 0.0f, 1.0f) }, // 4
		//	{ glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(0.0f, 1.0f, 1.0f) }, // 5
		//	{ glm::vec3(1.0f,  1.0f,  1.0f), glm::vec3(1.0f, 1.0f, 1.0f) }, // 6
		//	{ glm::vec3(1.0f, -1.0f,  1.0f), glm::vec3(1.0f, 0.0f, 1.0f) }  // 7
		//};

		//static WORD g_Indicies[36] =
		//{
		//	0, 1, 2, 0, 2, 3,
		//	4, 6, 5, 4, 7, 6,
		//	4, 5, 1, 4, 1, 0,
		//	3, 2, 6, 3, 6, 7,
		//	1, 5, 6, 1, 6, 2,
		//	4, 0, 3, 4, 3, 7
		//};

	}
}