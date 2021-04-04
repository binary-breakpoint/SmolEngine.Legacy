#include "stdafx.h"
#include "Common/Events.h"

namespace Frostium
{
	void EventSender::SendEvent(Event& event, EventType eventType, EventCategory eventCategory, int action, int key)
	{
		event.m_EventType = eventType; event.m_EventCategory = eventCategory; event.m_Key = key; event.m_Action = action; event.m_Handled = false;
		OnEventFn(event);
	}

	bool Event::IsType(EventType type)
	{
		return m_EventType == type;
	}

	bool Event::IsCategory(EventCategory category)
	{
		return m_EventCategory == category;
	}

	bool Event::IsEventReceived(Frostium::EventType type, Event& event)
	{
		return event.m_EventType == type;
	}
}
