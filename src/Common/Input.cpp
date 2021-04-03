#include "stdafx.h"
#include "Common/Input.h"
#include "GraphicsContext.h"

#include <GLFW/glfw3.h>

namespace Frostium
{
	bool Input::IsKeyPressed(KeyCode key)
	{
		GLFWwindow* window = GraphicsContext::GetSingleton()->GetNativeWindow();
		auto state = glfwGetKey(window, static_cast<int32_t>(key));
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonPressed(MouseCode button)
	{
		GLFWwindow* window = GraphicsContext::GetSingleton()->GetNativeWindow();
		auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
		return state == GLFW_PRESS;
	}

	float Input::GetMouseX()
	{
		auto [x, y] = GetMousePosition();
		return x;
	}

	float Input::GetMouseY()
	{
		auto [x, y] = GetMousePosition();
		return y;
	}

	bool Input::IsEventReceived(EventType type, Event& event)
	{
		return event.m_EventType == (unsigned int)type;
	}

	std::pair<float, float> Input::GetMousePosition()
	{
		GLFWwindow* window = GraphicsContext::GetSingleton()->GetNativeWindow();
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}
}