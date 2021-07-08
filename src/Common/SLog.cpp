#include "stdafx.h"
#include "Common/SLog.h"
#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	SLog::SLog(const std::string& name)
	{
		s_Instance = this;

		spdlog::set_pattern("%^[%T] %n: %v%$");
#ifdef FROSTIUM_SMOLENGINE_IMPL
		s_Instance->m_Logger = spdlog::stdout_color_mt("SmolEngine");
#else
		s_Instance->m_Logger = spdlog::stdout_color_mt(name.c_str());
#endif

		s_Instance->m_Logger->set_level(spdlog::level::trace);
	}

	SLog::~SLog()
	{
		s_Instance = nullptr;
	}

	void SLog::SetOnPrintCallback(const std::function<void(const std::string&&, LogType)>& callback)
	{
		m_Callback = callback;
	}

	spdlog::logger* SLog::GetLogger()
	{
		return s_Instance->m_Logger.get();
	}
}
