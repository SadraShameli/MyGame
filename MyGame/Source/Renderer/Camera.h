#pragma once

#include "Camera.h"

#include "../Core/Timer.h"

#include "../Events/Event.h"
#include "../Events/EventCodes/KeyCodes.h"
#include "../Events/EventCodes/MouseCodes.h"
#include "../Events/MouseEvent.h"

namespace MyGame
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		void SetViewPort(float width, float height) { m_Width = width; m_Height = height; m_AspectRatio = width / height; };
		void SetDistance(float distance) { m_Distance = distance; }

		void SetPosition(float x, float y, float z) { DirectX::XMFLOAT3 rot = { x, y, z }; m_Position = DirectX::XMLoadFloat3(&rot); }
		void SetRotation(float x, float y, float z) { DirectX::XMFLOAT3 rot = { x, y, z }; m_Rotation = DirectX::XMLoadFloat3(&rot); }
		void SetFocalPoint(float x, float y, float z) { DirectX::XMFLOAT3 rot = { x, y, z }; m_FocalPoint = DirectX::XMLoadFloat3(&rot); }
		void SetPosition(const DirectX::XMVECTOR& vec) { m_Position = vec; }
		void SetRotation(const DirectX::XMVECTOR& vec) { m_Rotation = vec; }
		void SetFocalPoint(const DirectX::XMVECTOR& vec) { m_FocalPoint = vec; }

		void SetView(const DirectX::XMMATRIX& matrix) { m_ViewMatrix = matrix; }
		void SetProjection(const DirectX::XMMATRIX& matrix) { m_ProjectionMatrix = matrix; }

		float GetYaw() const { return m_Yaw; }
		float GetPitch() const { return m_Pitch; }
		float GetDistance() const { return m_Distance; }

		const DirectX::XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }
		const DirectX::XMMATRIX& GetProjectionMatrix() const { return m_ProjectionMatrix; }
		DirectX::XMMATRIX GetViewProjection() const { return  m_ViewMatrix * m_ProjectionMatrix; }

		DirectX::XMVECTOR GetUpDirection() const;
		DirectX::XMVECTOR GetRightDirection() const;
		DirectX::XMVECTOR GetForwardDirection() const;
		DirectX::XMVECTOR GetOrientation() const;
		const DirectX::XMVECTOR& GetPosition() const { return m_Position; }

	private:
		void UpdateView();
		void UpdateProjection();

		bool OnMouseScroll(MouseScrolledEvent& e);
		void MousePan(const DirectX::XMFLOAT2& delta);
		void MouseRotate(const DirectX::XMFLOAT2& delta);
		void MouseZoom(float delta);

		DirectX::XMVECTOR CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		float m_FOV = 45.0f, m_NearClip = 0.1f, m_FarClip = 1000.0f;
		float m_Width = 0, m_Height = 0, m_AspectRatio = 1.778f;

		float m_Distance = 3.0f;
		float m_Pitch = 1.0f, m_Yaw = 1.0f;

		DirectX::XMMATRIX m_ViewMatrix = {};
		DirectX::XMMATRIX m_ProjectionMatrix = {};

		DirectX::XMVECTOR m_Position = {};
		DirectX::XMVECTOR m_Rotation = {};
		DirectX::XMVECTOR m_FocalPoint = {};
		DirectX::XMVECTOR m_InitialMousePosition = {};
	};
}
