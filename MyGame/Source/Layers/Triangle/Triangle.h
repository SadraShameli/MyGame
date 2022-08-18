#pragma once

#include "../../Core/Layer.h"

#include "../../Events/AppEvent.h"
#include "../../Events/EventCodes/KeyCodes.h"
#include "../../Events/EventCodes/MouseCodes.h"

#include "../../Core/Timer.h"

namespace MyGame
{
	class TriangleLayer : public Layer
	{
	public:
		TriangleLayer();
		~TriangleLayer() = default;

		virtual void OnAttach();
		virtual void OnDetach();
		virtual void OnUpdate(Timestep ts);
		virtual void OnEvent(Event& e);

		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		bool m_BlockEvents = true;
	};
}
