#pragma once

#include <DirectXMath.h>

namespace MyGame
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const DirectX::XMMATRIX projection) : m_Projection(projection) {}

		virtual ~Camera() = default;

		const DirectX::XMMATRIX& GetProjection() const { return m_Projection; }

	protected:
		DirectX::XMMATRIX m_Projection = DirectX::XMMatrixIdentity();
	};
}