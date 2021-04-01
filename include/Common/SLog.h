#pragma once
#include "Common/Core.h"
extern "C++"
{
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
}

namespace Frostium
{
	class SMOL_ENGINE_API SLog
	{
	public:

		static void InitLog();

		///

		inline static std::shared_ptr<spdlog::logger>& GetNativeLogger() { return s_NativeLogger; }

		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		inline static std::shared_ptr<spdlog::logger>& GetEditorLogger() { return s_EditorLogger; }

	private:

		static std::shared_ptr<spdlog::logger> s_NativeLogger;

		static std::shared_ptr<spdlog::logger> s_ClientLogger;

		static std::shared_ptr<spdlog::logger> s_EditorLogger;
	};
}

//#define INIT_LOG ::Frostium::SLog::InitLog()

//---------------------------------------------Native Logger-------------------------------------------------------------------

#define NATIVE_ERROR(...) ::Frostium::SLog::GetNativeLogger()->error(__VA_ARGS__)
#define NATIVE_WARN(...)  ::Frostium::SLog::GetNativeLogger()->warn(__VA_ARGS__)
#define NATIVE_INFO(...)  ::Frostium::SLog::GetNativeLogger()->trace(__VA_ARGS__)

//---------------------------------------------Native Logger-------------------------------------------------------------------



//---------------------------------------------Client Logger-------------------------------------------------------------------

#define CLIENT_ERROR(...)      ::Frostium::SLog::GetClientLogger()->error(__VA_ARGS__)
#define CLIENT_WARN(...)       ::Frostium::SLog::GetClientLogger()->warn(__VA_ARGS__)
#define CLIENT_INFO(...)       ::Frostium::SLog::GetClientLogger()->trace(__VA_ARGS__)

//---------------------------------------------Client Logger-------------------------------------------------------------------


//----------------------------------------------Editor Logger-------------------------------------------------------------------

#define EDITOR_ERROR(...)      ::Frostium::SLog::GetEditorLogger()->error(__VA_ARGS__)
#define EDITOR_WARN(...)       ::Frostium::SLog::GetEditorLogger()->warn(__VA_ARGS__)
#define EDITOR_INFO(...)       ::Frostium::SLog::GetEditorLogger()->trace(__VA_ARGS__)

//----------------------------------------------Editor Logger-------------------------------------------------------------------


