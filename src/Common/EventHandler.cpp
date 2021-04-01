#include "stdafx.h"
#include "Common/EventHandler.h"
#include "Common/InputEvent.h"
#include "Common/ApplicationEvent.h"
#include "Common/MouseEvent.h"

namespace Frostium 
{
	void EventHandler::SendEvent(Event& event, EventType eventType, EventCategory eventCategory, int action, int key)
	{
		event.m_EventType = (int)eventType; event.m_EventCategory = (int)eventCategory; event.m_Key = key; event.m_Action = action; event.m_Handled = false;
		OnEventFn(event);
	}
}
