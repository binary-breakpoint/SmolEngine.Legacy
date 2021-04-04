#include "stdafx.h"
#include "Common/SLog.h"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Frostium
{
	std::shared_ptr<spdlog::logger> SLog::s_NativeLogger;

	void SLog::InitLog()
	{
		spdlog::set_pattern("%^[%T] %n: %v%$");
		s_NativeLogger = spdlog::stdout_color_mt("Frostium");
		s_NativeLogger->set_level(spdlog::level::trace);
	}
}
