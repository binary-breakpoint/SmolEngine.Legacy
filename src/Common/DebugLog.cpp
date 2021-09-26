#include "stdafx.h"
#include "Common/DebugLog.h"

namespace SmolEngine
{
	DebugLog::DebugLog(const std::function<void(const std::string&&, LogLevel)>& callback)
		: m_Callback(callback)
	{
		s_Instance = this;
	}

	DebugLog::~DebugLog()
	{
		s_Instance = nullptr;
	}
}
