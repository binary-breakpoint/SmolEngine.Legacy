#pragma once
#include "Common/Core.h"
extern "C++"
{
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
}

namespace Frostium
{
	class SLog
	{
	public:

		static void InitLog();
		inline static std::shared_ptr<spdlog::logger>& GetNativeLogger() { return s_NativeLogger; }

	private:

		static std::shared_ptr<spdlog::logger> s_NativeLogger;
	};
}

#define NATIVE_ERROR(...) ::Frostium::SLog::GetNativeLogger()->error(__VA_ARGS__)
#define NATIVE_WARN(...)  ::Frostium::SLog::GetNativeLogger()->warn(__VA_ARGS__)
#define NATIVE_INFO(...)  ::Frostium::SLog::GetNativeLogger()->trace(__VA_ARGS__)



