#include "CommonHeaders.h"

#include "Camera.h"

#include "../Core/Base.h"
#include "../Core/Input.h"

using namespace DirectX;

namespace MyGame
{
	Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip),
		m_ProjectionMatrix(XMMatrixPerspectiveFovLH(XMConvertToRadians(fov), aspectRatio, nearClip, farClip))
	{
		UpdateView();
	}

	void Camera::UpdateView()
	{
		//m_ViewMatrix = XMMatrixInverse(nullptr, XMMatrixTranslationFromVector(CalculatePosition()) * XMMatrixRotationQuaternion(GetOrientation()));
		m_ViewMatrix = XMMatrixLookAtLH(CalculatePosition(), m_FocalPoint, GetUpDirection());

		MYGAME_INFO("{0} || {1} || {2}", m_Pitch, m_Yaw, m_Distance);
	}

	void Camera::UpdateProjection()
	{
		m_AspectRatio = m_Width / m_Height;
		m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	std::pair<float, float> Camera::PanSpeed() const
	{
		float x = (std::min)(m_Width / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = (std::min)(m_Height / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float Camera::RotationSpeed() const
	{
		return 0.8f;
	}

	float Camera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = (std::max)(distance, 0.0f);

		float speed = distance * distance;
		speed = (std::min)(speed, 100.0f); // max speed = 100
		return speed;
	}

	void Camera::OnUpdate(Timestep ts)
	{
		if (Input::IsKeyPressed(Key::LeftAlt))
		{
			XMVECTOR mouse = { Input::GetMouseX(), Input::GetMouseY() };
			XMFLOAT2 delta = {};
			XMStoreFloat2(&delta, (mouse - m_InitialMousePosition) * 0.008f);
			m_InitialMousePosition = mouse;

			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				MouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				MouseZoom(delta.y);
		}

		if (Input::IsKeyPressed(Key::R))
		{
			m_Pitch = m_Yaw = 1.0f;
			m_Distance = 3.0f;
		}

		UpdateView();
	}

	void Camera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(MYGAME_BIND_FN(Camera::OnMouseScroll));
	}

	bool Camera::OnMouseScroll(MouseScrolledEvent& e)
	{
		float delta = e.GetOffset() * 0.008f;
		MouseZoom(delta);
		UpdateView();
		return false;
	}

	void Camera::MousePan(const XMFLOAT2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocalPoint += GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocalPoint += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void Camera::MouseRotate(const XMFLOAT2& delta)
	{
		float yawSign = XMVectorGetY(GetUpDirection()) < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void Camera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocalPoint += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	XMVECTOR Camera::GetUpDirection() const { return XMVector3Rotate(GetOrientation(), { 0.0f, 1.0f, 0.0f }); }
	XMVECTOR Camera::GetRightDirection() const { return XMVector3Rotate(GetOrientation(), { 1.0f, 0.0f, 0.0f }); }
	XMVECTOR Camera::GetForwardDirection() const { return XMVector3Rotate(GetOrientation(), { 0.0f, 0.0f, -1.0f }); }
	XMVECTOR Camera::CalculatePosition() const { return m_FocalPoint - GetForwardDirection() * m_Distance; }
	XMVECTOR Camera::GetOrientation() const { return { -m_Pitch, -m_Yaw, 0.0f }; }

}
