#include "CommonHeaders.h"

#include "OrthographicCamera.h"

#include "../Debugs/Instrumentor.h"

using namespace DirectX;

namespace MyGame
{
	OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
		: m_ProjectionMatrix(XMMatrixOrthographicLH(left, right, bottom, top)), m_ViewMatrix(XMMatrixIdentity())
	{
		MYGAME_PROFILE_FUNCTION();

		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
	{
		MYGAME_PROFILE_FUNCTION();

		m_ProjectionMatrix = XMMatrixOrthographicLH(left, right, bottom, top);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		MYGAME_PROFILE_FUNCTION();

		XMMATRIX transform = XMMatrixTranslationFromVector(XMLoadFloat3(&m_Position)) * XMMatrixRotationAxis({ 0,0,1 }, XMConvertToRadians(m_Rotation));
		m_ViewMatrix = XMMatrixInverse(nullptr, transform);
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}