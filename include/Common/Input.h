#pragma once

#include "Common/EventHandler.h"
#include "InputCodes.h"

namespace Frostium 
{
	class Input
	{
	public:

		static bool IsKeyPressed(KeyCode key);

		static bool IsMouseButtonPressed(MouseCode button);

		static bool IsEventReceived(Frostium::EventType type, Event& event);

		static float GetMouseX();

		static float GetMouseY();

		static std::pair<float, float> GetMousePosition();
	};

}