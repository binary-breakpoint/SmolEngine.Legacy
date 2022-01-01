#include "Debug/DebugLog.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace SmolEngine
{
	DebugLog* DebugLog::s_Instance = new DebugLog();

	DebugLog::DebugLog()
	{
		s_Instance = this;

		spdlog::set_pattern("%^[%T] %n: %v%$");

		s_Instance->m_Logger = spdlog::stdout_color_mt("Engine");
		s_Instance->m_Logger->set_level(spdlog::level::trace);
	}

	DebugLog::~DebugLog()
	{
		s_Instance = nullptr;
	}
}