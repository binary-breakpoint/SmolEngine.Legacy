#include "stdafx.h"
#include "Common/Window.h"
#include "Common/Events.h"
#include "Common/SLog.h"

#include <GLFW/glfw3.h>
#include <glad\glad.h>

namespace Frostium 
{
	static WindowData* DataPtr = nullptr;
	static EventSender* EventHandler = nullptr;

	void Window::Init(const WindowCreateInfo* info, EventSender* event_hadler)
	{
		DataPtr = &m_Data;
		EventHandler = event_hadler;

		m_Data.Title = info->Title;
		m_Data.Height = info->Height;
		m_Data.Width = info->Width;
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
		EventHandler->SendEvent(resizeEvent, EventType::WINDOW_RESIZE, EventCategory::EVENT_APP);
	}

	void Window::SetHeight(uint32_t value)
	{
		m_Data.Height = value;
		WindowResizeEvent resizeEvent(m_Data);
		EventHandler->SendEvent(resizeEvent, EventType::WINDOW_RESIZE, EventCategory::EVENT_APP);
	}

	GLFWwindow* Window::GetNativeWindow() const
	{
		return m_Window;
	}

	void Window::SetTitle(const std::string& name)
	{
		glfwSetWindowTitle(m_Window, name.c_str());
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
				DataPtr->Width = width;

				WindowResizeEvent resizeEvent(*DataPtr);
				EventHandler->SendEvent(resizeEvent, EventType::WINDOW_RESIZE, EventCategory::EVENT_APP);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) 
		{
				WindowCloseEvent closeEvent;
				EventHandler->SendEvent(closeEvent, EventType::WINDOW_CLOSE, EventCategory::EVENT_APP);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow * window, int key, int scancode, int action, int mods)
		{
				KeyEvent keyEvent;

				switch (action)
				{
				case GLFW_PRESS:
				{
					EventHandler->SendEvent(keyEvent, EventType::KEY_PRESS, EventCategory::EVENT_KEYBOARD, action, key);
					break;
				}
				case GLFW_RELEASE:
				{
					EventHandler->SendEvent(keyEvent, EventType::KEY_RELEASE, EventCategory::EVENT_KEYBOARD, action, key);
					break;
				}
				case GLFW_REPEAT:
				{
					EventHandler->SendEvent(keyEvent, EventType::KEY_REPEAT, EventCategory::EVENT_KEYBOARD, action, key);
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
					EventHandler->SendEvent(mouseButton, EventType::MOUSE_PRESS, EventCategory::EVENT_MOUSE, action, button);
					break;
				}
				case GLFW_RELEASE:
				{
					EventHandler->SendEvent(mouseButton, EventType::MOUSE_RELEASE, EventCategory::EVENT_MOUSE, action, button);
					break;
				}
				}
				
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
				MouseScrollEvent scrollEvent(static_cast<float>(xOffset), static_cast<float>(yOffset));
				EventHandler->SendEvent(scrollEvent, EventType::MOUSE_SCROLL, EventCategory::EVENT_MOUSE);
		});
	

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) 
		{
				MouseMoveEvent mouseEvent(xPos, yPos);
				EventHandler->SendEvent(mouseEvent, EventType::MOUSE_MOVE, EventCategory::EVENT_MOUSE);
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