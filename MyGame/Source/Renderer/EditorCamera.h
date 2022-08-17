#pragma once

#include "Camera.h"

#include "../Core/Timer.h"

#include "../Events/Event.h"
#include "../Events/EventCodes/KeyCodes.h"
#include "../Events/EventCodes/MouseCodes.h"
#include "../Events/MouseEvent.h"

namespace MyGame
{
	class EditorCamera : public Camera
	{
	public:
		EditorCamera(float fov, float aspectRatio, float nearClip, float farClip);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		inline float GetDistance() const { return m_Distance; }
		inline void SetDistance(float distance) { m_Distance = distance; }
		inline void SetViewportSize(float width, float height) { m_ViewportWidth = width; m_ViewportHeight = height; UpdateProjection(); }

		const DirectX::XMMATRIX& GetViewMatrix() const { return m_ViewMatrix; }
		DirectX::XMMATRIX GetViewProjection() const { return m_Projection * m_ViewMatrix; }

		DirectX::XMVECTOR GetUpDirection() const;
		DirectX::XMVECTOR GetRightDirection() const;
		DirectX::XMVECTOR GetForwardDirection() const;
		const DirectX::XMVECTOR& GetPosition() const { return m_Position; }
		DirectX::XMVECTOR GetOrientation() const;

		float GetPitch() const { return m_Pitch; }
		float GetYaw() const { return m_Yaw; }

	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScroll(MouseScrolledEvent& e);
		void MousePan(const DirectX::XMFLOAT2& delta);
		void MouseRotate(const DirectX::XMFLOAT2& delta);
		void MouseZoom(float delta);

		DirectX::XMVECTOR CalculatePosition() const;

		std::pair<float, float> PanSpeed() const;
		float RotationSpeed() const;
		float ZoomSpeed() const;

	private:
		float m_FOV = 45.0f, m_AspectRatio = 1.778f, m_NearClip = 0.1f, m_FarClip = 1000.0f;

		DirectX::XMMATRIX m_ViewMatrix = {};
		DirectX::XMVECTOR m_Position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMVECTOR m_FocalPoint = { 0.0f, 0.0f, 0.0f };
		DirectX::XMVECTOR m_InitialMousePosition = { 0.0f, 0.0f };

		float m_Distance = 10.0f;
		float m_Pitch = 0.0f, m_Yaw = 0.0f;
		float m_ViewportWidth = 1280, m_ViewportHeight = 720;
	};
}
