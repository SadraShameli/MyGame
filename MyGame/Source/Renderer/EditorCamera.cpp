#include "CommonHeaders.h"

#include "EditorCamera.h"

#include "../Core/Base.h"
#include "../Core/Input.h"

using namespace DirectX;

namespace MyGame
{
	EditorCamera::EditorCamera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), Camera(XMMatrixPerspectiveFovLH(XMConvertToRadians(fov), aspectRatio, nearClip, farClip))
	{
		UpdateView();
	}

	void EditorCamera::UpdateProjection()
	{
		m_AspectRatio = m_ViewportWidth / m_ViewportHeight;
		m_Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	void EditorCamera::UpdateView()
	{
		// m_Yaw = m_Pitch = 0.0f; // Lock the camera's rotation

		XMVECTOR orientation = GetOrientation();
		m_ViewMatrix = XMMatrixTranslationFromVector(CalculatePosition()) * XMMatrixRotationQuaternion(orientation);
		m_ViewMatrix = XMMatrixInverse(nullptr, m_ViewMatrix);
	}

	std::pair<float, float> EditorCamera::PanSpeed() const
	{
		float x = (std::min)(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = (std::min)(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float EditorCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float EditorCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = (std::max)(distance, 0.0f);

		float speed = distance * distance;
		speed = (std::min)(speed, 100.0f); // max speed = 100
		return speed;
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		if (Input::IsKeyPressed(Key::LeftAlt))
		{
			const XMVECTOR& mouse{ Input::GetMouseX(), Input::GetMouseY() };
			XMFLOAT2 delta = {};
			XMStoreFloat2(&delta, (mouse - m_InitialMousePosition) * 0.003f);
			m_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				MouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				MouseZoom(delta.y);
		}

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e) { MYGAME_BIND_FN(EditorCamera::OnMouseScroll); }

	bool EditorCamera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetYOffset() * 0.1f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void EditorCamera::MousePan(const XMFLOAT2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void EditorCamera::MouseRotate(const XMFLOAT2& delta)
	{
		XMFLOAT3 upDirection = {};
		XMStoreFloat3(&upDirection, GetUpDirection());
		float yawSign = upDirection.y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void EditorCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	XMVECTOR EditorCamera::GetUpDirection() const { return XMVector3Rotate(GetOrientation(), { 0.0f, 1.0f, 0.0f }); }
	XMVECTOR EditorCamera::GetRightDirection() const { return XMVector3Rotate(GetOrientation(), { 1.0f, 0.0f, 0.0f }); }
	XMVECTOR EditorCamera::GetForwardDirection() const { return XMVector3Rotate(GetOrientation(), { 0.0f, 0.0f, -1.0f }); }
	XMVECTOR EditorCamera::CalculatePosition() const { return m_FocalPoint - GetForwardDirection() * m_Distance; }
	XMVECTOR EditorCamera::GetOrientation() const { return { -m_Pitch, -m_Yaw, 0.0f }; }
}
