#pragma once

#include "../Core/Time.h"
#include "../Events/Event.h"
#include "../DirectX/DirectXImpl.h"

namespace MyGame
{
	class Layer : public DirectXImpl
	{
	public:
		Layer(const std::string& name = "Layer") { m_DebugName = name; };
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnEvent(Event& event) {}
		virtual void OnImGuiRender() {}

		const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};
}