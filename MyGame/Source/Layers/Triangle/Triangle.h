#pragma once

#include "../../Core/Layer.h"

#include "../../Events/AppEvent.h"
#include "../../Events/EventCodes/KeyCodes.h"
#include "../../Events/EventCodes/MouseCodes.h"

#include "../../Core/Timer.h"

namespace MyGame
{
	inline float x, y, z, roll, yaw, pitch;

	class TriangleLayer : public Layer
	{
	public:
		TriangleLayer();
		~TriangleLayer() = default;

		void OnAttach();
		void OnDetach();
		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);
		void OnImGuiRender();

		void BlockEvents(bool block) { m_BlockEvents = block; }

	private:
		bool m_BlockEvents = true;
	};
}
