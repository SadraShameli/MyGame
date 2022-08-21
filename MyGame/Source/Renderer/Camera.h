#pragma once

#include "Camera.h"

#include "../Core/Timer.h"

#include "../Events/Event.h"
#include "../Events/EventCodes/KeyCodes.h"
#include "../Events/EventCodes/MouseCodes.h"
#include "../Events/MouseEvent.h"

namespace MyGame
{
	class BaseCamera
	{
	public:
		BaseCamera() = default;
		BaseCamera(const DirectX::XMMATRIX& projection) : m_Projection(projection) {}

		virtual ~BaseCamera() = default;

		const DirectX::XMMATRIX& GetProjection() const { return m_Projection; }

	protected:
		DirectX::XMMATRIX m_Projection = {};
	};

	class Camera
	{
	public:
		Camera() = default;
		Camera(float fov, float width, float height, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		void UpdateView();
		void UpdateProjection();

		void SetViewPort(float width, float height) { m_Width = width; m_Height = height; m_AspectRatio = width / height; };
		void SetDistance(float distance) { Distance = distance; }

		void SetRotation(float roll, float pitch, float yaw) { Roll = roll; Pitch = pitch; Yaw = yaw; }
		void SetFocalPoint(float x, float y, float z) { FocalPoint = { x, y, z }; }

		DirectX::XMMATRIX GetViewProjection() const { return ViewMatrix * Projection; }

		DirectX::XMVECTOR GetUpDirection();
		DirectX::XMVECTOR GetRightDirection();
		DirectX::XMVECTOR GetForwardDirection();
		DirectX::XMVECTOR GetOrientation();

	public:
		float Distance = 3.0f, FOV = 45.0f;
		float Roll = 0.0f, Pitch = 0.0f, Yaw = 0.0f;

		DirectX::XMVECTOR FocalPoint;
		DirectX::XMMATRIX ViewMatrix;
		DirectX::XMMATRIX Projection;

	private:
		bool OnMouseScroll(MouseScrolledEvent& e);
		void MousePan(const DirectX::XMFLOAT2& delta);
		void MouseRotate(const DirectX::XMFLOAT2& delta);
		void MouseZoom(float delta);

		float RotationSpeed();
		float ZoomSpeed();

		DirectX::XMVECTOR CalculatePosition();
		DirectX::XMFLOAT2 PanSpeed();

	private:
		float m_NearClip = 0.1f, m_FarClip = 1000.0f;
		float m_Width = 0.0f, m_Height = 0.0f, m_AspectRatio = 1.778f;

		DirectX::XMVECTOR m_InitialMousePosition = {};
	};
}
