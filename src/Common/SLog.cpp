#include "stdafx.h"
#include "Common/SLog.h"
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	SLog* SLog::s_Instance = new SLog();

	SLog::SLog()
	{
		s_Instance = this;
		InitLog();
	}

	SLog::~SLog()
	{
		s_Instance = nullptr;
	}

	void SLog::InitLog()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_Instance->m_NativeLogger = spdlog::stdout_color_mt("Engine");
		s_Instance->m_NativeLogger->set_level(spdlog::level::trace);
	}

	spdlog::logger* SLog::GetNativeLogger()
	{
		return s_Instance->m_NativeLogger.get();
	}
}
