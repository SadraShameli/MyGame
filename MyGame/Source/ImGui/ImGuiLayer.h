#pragma once

#include "Source/Core/Layer.h"

#include "Source/Events/AppEvent.h"
#include "Source/Events/EventCodes/KeyCodes.h"
#include "Source/Events/EventCodes/MouseCodes.h"

namespace MyGame
{
	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }

		void SetLightMode();
		void SetDarkMode();

	private:
		bool m_BlockEvents = true;
	};
}
