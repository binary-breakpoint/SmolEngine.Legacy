#include "stdafx.h"
#include "Common/Window.h"
#include "Common/ApplicationEvent.h"
#include "Common/MouseEvent.h"
#include "Common/InputEvent.h"

#include <GLFW/glfw3.h>
#include <glad\glad.h>


namespace Frostium 
{
	static WindowData* DataPtr = nullptr;

	void Window::Init(const WindowCreateInfo* info)
	{
		if (!info->EventHandler)
			std::runtime_error("EventHandler is nullptr!");

		DataPtr = &m_Data;
		m_Data.Title = info->Title;
		m_Data.Height = info->Height;
		m_Data.Width = info->Width;
		m_Data.EventHandler = info->EventHandler;

		Create(info);
	}

	void Window::ProcessEvents()
	{
		if (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
		}
	}

	void Window::SetWidth(uint32_t value)
	{
		m_Data.Width = value;
		WindowResizeEvent resizeEvent(m_Data);
		DataPtr->EventHandler->SendEvent(resizeEvent, EventType::S_WINDOW_RESIZE, EventCategory::S_EVENT_APP);
	}

	void Window::SetHeight(uint32_t value)
	{
		m_Data.Height = value;
		WindowResizeEvent resizeEvent(m_Data);
		DataPtr->EventHandler->SendEvent(resizeEvent, EventType::S_WINDOW_RESIZE, EventCategory::S_EVENT_APP);
	}

	GLFWwindow* Window::GetNativeWindow() const
	{
		return m_Window;
	}

	WindowData* Window::GetWindowData()
	{
		return &m_Data;
	}

	uint32_t Window::GetWidth() const
	{
		return m_Data.Width;
	}

	uint32_t Window::GetHeight() const
	{
		return m_Data.Height;
	}

	void Window::Create(const WindowCreateInfo* info)
	{
		glfwInit();
		glfwSetErrorCallback([](int error, const char* description) { NATIVE_ERROR("GLFW Error ({0}): {1}", error, description); });

#ifndef FROSTIUM_OPENGL_IMPL
		// No need to create OpenGL window automatically
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
#endif //!FROSTIUM_OPENGL_IMPL

		// Create Window
		m_Window = glfwCreateWindow((int)info->Width, (int)info->Height, info->Title.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			std::runtime_error("Failed to create window!");
		}

		if (info->bFullscreen)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			glfwSetWindowMonitor(m_Window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
		}

		if (info->bVSync)
		{
			SetVSync(true);
		}


		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
		{
				DataPtr->Height = height;
				DataPtr->Width = height;

				WindowResizeEvent resizeEvent(*DataPtr);
				DataPtr->EventHandler->SendEvent(resizeEvent, EventType::S_WINDOW_RESIZE, EventCategory::S_EVENT_APP);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) 
		{
				WindowCloseEvent closeEvent;
				DataPtr->EventHandler->SendEvent(closeEvent, EventType::S_WINDOW_CLOSE, EventCategory::S_EVENT_APP);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow * window, int key, int scancode, int action, int mods)
		{
				KeyEvent keyEvent;

				switch (action)
				{
				case GLFW_PRESS:
				{
					DataPtr->EventHandler->SendEvent(keyEvent, EventType::S_KEY_PRESS, EventCategory::S_EVENT_KEYBOARD, action, key);
					break;
				}
				case GLFW_RELEASE:
				{
					DataPtr->EventHandler->SendEvent(keyEvent, EventType::S_KEY_RELEASE, EventCategory::S_EVENT_KEYBOARD, action, key);
					break;
				}
				case GLFW_REPEAT:
				{
					DataPtr->EventHandler->SendEvent(keyEvent, EventType::S_KEY_REPEAT, EventCategory::S_EVENT_KEYBOARD, action, key);
					break;
				}
				default:
					break;
				}
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) 
		{
				MouseButtonEvent mouseButton(button);

				switch (action)
				{
				case GLFW_PRESS:
				{
					DataPtr->EventHandler->SendEvent(mouseButton, EventType::S_MOUSE_PRESS, EventCategory::S_EVENT_MOUSE, action, button);
					break;
				}
				case GLFW_RELEASE:
				{
					DataPtr->EventHandler->SendEvent(mouseButton, EventType::S_MOUSE_RELEASE, EventCategory::S_EVENT_MOUSE, action, button);
					break;
				}
				}
				
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
				MouseScrollEvent scrollEvent(static_cast<float>(xOffset), static_cast<float>(yOffset));
				DataPtr->EventHandler->SendEvent(scrollEvent, EventType::S_MOUSE_SCROLL, EventCategory::S_EVENT_MOUSE);
		});
	

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) 
		{
				MouseMoveEvent mouseEvent(xPos, yPos);
				DataPtr->EventHandler->SendEvent(mouseEvent, EventType::S_MOUSE_MOVE, EventCategory::S_EVENT_MOUSE);
		});

	}

	void Window::ShutDown()
	{
		glfwDestroyWindow(m_Window);
	}

	void Window::SetVSync(bool enabled)
	{
		NATIVE_INFO("VSync enabled: {0}", enabled);
		enabled ? glfwSwapInterval(1) : glfwSwapInterval(0);
	}
}