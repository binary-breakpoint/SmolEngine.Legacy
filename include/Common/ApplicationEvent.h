#pragma once
#include "Common/SLog.h"
#include "Common/EventHandler.h"
#include "Common/SLog.h"
#include "Common/Window.h"

namespace Frostium 
{
	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(WindowData& data):
			m_Data(data)
		{ 

		}

		inline const unsigned int GetWidth() { return m_Data.Width; }
		inline const unsigned int GetHeight() { return m_Data.Height; }

	private:
		WindowData& m_Data;
	};

	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent()
		{

		}
	};
}
