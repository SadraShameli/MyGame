#include "CommonHeaders.h"

#include "Camera.h"

#include "../Core/Base.h"
#include "../Core/Input.h"

using namespace DirectX;

namespace MyGame
{
	Camera::Camera(float fov, float width, float height, float nearClip, float farClip)
		: FOV(fov), m_AspectRatio(width / height), m_NearClip(nearClip), m_FarClip(farClip),
		Projection(XMMatrixPerspectiveFovLH(XMConvertToRadians(fov), width / height, nearClip, farClip))
	{
		UpdateView();
	}

	void Camera::UpdateView()
	{
		ViewMatrix = XMMatrixTranslationFromVector(CalculatePosition()) * XMMatrixRotationQuaternion(GetOrientation());
	}

	void Camera::UpdateProjection()
	{
		Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(FOV), m_AspectRatio, m_NearClip, m_FarClip);
	}

	XMFLOAT2 Camera::PanSpeed()
	{
		float x = (std::min)(m_Width / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = (std::min)(m_Height / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float Camera::RotationSpeed()
	{
		return 0.8f;
	}

	float Camera::ZoomSpeed()
	{
		float distance = Distance * 0.2f;
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
			Pitch = Yaw = 1.0f;
			Distance = 3.0f;
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
		FocalPoint += -GetRightDirection() * delta.x * xSpeed * Distance;
		FocalPoint += GetUpDirection() * delta.y * ySpeed * Distance;
	}

	void Camera::MouseRotate(const XMFLOAT2& delta)
	{
		float yawSign = XMVectorGetY(GetUpDirection()) < 0 ? -1.0f : 1.0f;
		Yaw += yawSign * delta.x * RotationSpeed();
		Pitch += delta.y * RotationSpeed();
	}

	void Camera::MouseZoom(float delta)
	{
		Distance -= delta * ZoomSpeed();
		if (Distance < 1.0f)
		{
			FocalPoint += GetForwardDirection();
			Distance = 1.0f;
		}
	}

	XMVECTOR Camera::GetUpDirection()
	{
		return XMVector3Rotate({ 0.0f, 1.0f, 0.0f }, GetOrientation());
	}

	XMVECTOR Camera::GetRightDirection()
	{
		return XMVector3Rotate({ 1.0f, 0.0f, 0.0f }, GetOrientation());
	}

	XMVECTOR Camera::GetForwardDirection()
	{
		return XMVector3Rotate({ 0.0f, 0.0f, -1.0f }, GetOrientation());
	}

	XMVECTOR Camera::CalculatePosition()
	{
		return FocalPoint - GetForwardDirection() * Distance;
	}

	XMVECTOR Camera::GetOrientation()
	{
		return XMQuaternionRotationRollPitchYaw(-XMConvertToRadians(Pitch), -XMConvertToRadians(Yaw), 0.0f);
	}
}
