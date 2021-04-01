#pragma once

#include "Common/Core.h"
#include "Common/SLog.h"
#include "Common/EventHandler.h"

struct GLFWwindow;

namespace Frostium 
{

	struct WindowData
	{
		uint32_t            Width = 720;
		uint32_t            Height = 480;

		std::string         Title = "Some Application";
		Ref<EventHandler>   EventHandler = nullptr;
	};

	struct WindowCreateInfo
	{
		uint32_t            Width = 720;
		uint32_t            Height = 480;

		bool                bVSync = false;
		bool                bFullscreen = false;

		std::string         Title = "SomeApplication";
		Ref<EventHandler>   EventHandler = nullptr;
	};

	class Window
	{
	public:

		/// Main

		void Init(const WindowCreateInfo* info);

		void ProcessEvents();

		void ShutDown();

		/// Getters

		GLFWwindow* GetNativeWindow() const;

		WindowData* GetWindowData();

		uint32_t GetWidth() const;

		uint32_t GetHeight() const;

		/// Setters

		void SetWidth(uint32_t value);

		void SetHeight(uint32_t value);

	private:

		void Create(const WindowCreateInfo* info);

		void SetVSync(bool enabled);

	private:

		GLFWwindow* m_Window = nullptr;
		WindowData  m_Data;
	};

}
