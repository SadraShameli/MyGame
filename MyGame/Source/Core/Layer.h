#pragma once

#include "Source/Core/Base.h"
#include "Source/Core/Timestep.h"
#include "Source/Events/Event.h"

namespace MyGame {

	class Layer
	{
	public:
		Layer(const std::string& name = "Layer") { m_DebugName = name; };
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate(Timestep ts) {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		const std::string& GetName() const { return m_DebugName; }

	protected:
		std::string m_DebugName;
	};
}