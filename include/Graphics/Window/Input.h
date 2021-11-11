#pragma once

#include "Graphics/Window/Events.h"
#include "Graphics/Window/InputCodes.h"

namespace SmolEngine
{
	class Input
	{
	public:

		// Helpers
		static bool IsKeyPressed(KeyCode key);
		static bool IsMouseButtonPressed(MouseCode button);

		// Getters
		static float GetMouseX();
		static float GetMouseY();
		static std::pair<float, float> GetMousePosition();
	};

}