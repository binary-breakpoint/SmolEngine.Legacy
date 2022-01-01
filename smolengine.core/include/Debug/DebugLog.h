#pragma once
#include "Memory.h"

#include <sstream>
#include <mutex>

extern "C++"
{
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/stdout_color_sinks.h>
}

namespace SmolEngine
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
		DebugLog();
		~DebugLog();

		template<typename FormatString, typename... Args>
		static void LogError(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Error, std::forward<FormatString>(fmt), std::forward<Args>(args)...);
		}

		template<typename FormatString, typename... Args>
		static void LogInfo(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Info, std::forward<FormatString>(fmt), std::forward<Args>(args)...);
		}

		template<typename FormatString, typename... Args>
		static void LogWarn(FormatString&& fmt, Args&& ...args)
		{
			Log(LogLevel::Warning, std::forward<FormatString>(fmt), std::forward<Args>(args)...);
		}

		template<typename FormatString, typename... Args>
		static void Log(LogLevel level, FormatString&& fmt, Args&& ...args)
		{
			const std::lock_guard<std::mutex> lock(s_Instance->m_Mutex);

			switch (level)
			{
			case LogLevel::Info: s_Instance->m_Logger->trace(std::forward<FormatString>(fmt), std::forward<Args>(args)...); break;
			case LogLevel::Warning: s_Instance->m_Logger->warn(std::forward<FormatString>(fmt), std::forward<Args>(args)...); break;
			case LogLevel::Error: s_Instance->m_Logger->error(std::forward<FormatString>(fmt), std::forward<Args>(args)...); break;
			}

			const auto& callback = s_Instance->m_Callback;
			if (callback != nullptr)
			{
				spdlog::memory_buf_t buf;
				fmt::format_to(buf, fmt, args...);
				auto result = std::string(buf.data(), buf.size());
				callback(std::forward<std::string>(result), level);
			}
		}

		static void SetCallback(const std::function<void(const std::string&&, LogLevel)>& callback)
		{
			s_Instance->m_Callback = callback;
		}

	public:
		std::mutex m_Mutex{};
		static DebugLog* s_Instance;
		std::shared_ptr<spdlog::logger> m_Logger = nullptr;
		std::function<void(const std::string&&, LogLevel)> m_Callback = nullptr;
	};

#define RUNTIME_ERROR(msg, ...) DebugLog::LogError(msg, __VA_ARGS__); abort()
}


