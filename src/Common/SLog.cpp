#include "stdafx.h"
#include "Common/DebugLog.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	DebugLog::DebugLog()
	{
		s_Instance = this;
	}

	DebugLog::~DebugLog()
	{
		s_Instance = nullptr;
	}

	void DebugLog::SetCallback(const std::function<void(const std::string&&, LogLevel)>& callback)
	{
		m_Callback = callback;
	}
}
