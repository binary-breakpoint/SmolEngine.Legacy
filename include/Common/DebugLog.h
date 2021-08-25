#pragma once
#include "Common/Common.h"

#include <functional>
#include <sstream>

extern "C++"
{
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
}

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	enum class LogLevel
	{
		Info, 
		Warning, 
		Error
	};

	class DebugLog
	{
	public:
		DebugLog(const std::function<void(const std::string&&, LogLevel)>& callback);
	   ~DebugLog();

		template<typename FormatString, typename... Args>
		static void LogError(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Error, fmt, args...);
		}

		template<typename FormatString, typename... Args>
		static void LogInfo(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Info, fmt, args...);
		}

		template<typename FormatString, typename... Args>
		static void LogWarn(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Warning, fmt, args...);
		}

		template<typename FormatString, typename... Args>
		static void Log(LogLevel level, FormatString&& fmt, Args&& ...args)
		{
			if (s_Instance != nullptr)
			{
				auto& callback = s_Instance->m_Callback;
				if (callback != nullptr)
				{
					spdlog::memory_buf_t buf;
					fmt::format_to(buf, fmt, args...);
					auto result = std::string(buf.data(), buf.size());
					callback(std::forward<std::string>(result), level);
				}
			}
		}


	public:
		inline static DebugLog*                               s_Instance = nullptr;
		std::function<void(const std::string&&, LogLevel)>    m_Callback = nullptr;
	};
}



