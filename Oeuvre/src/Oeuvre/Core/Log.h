#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Oeuvre
{
	class Log
	{
	public:
		Log() = default;
		static void Initialize();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

// Core log macros
#define OV_CORE_TRACE(...)    ::Oeuvre::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define OV_CORE_INFO(...)     ::Oeuvre::Log::GetCoreLogger()->info(__VA_ARGS__)
#define OV_CORE_WARN(...)     ::Oeuvre::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define OV_CORE_ERROR(...)    ::Oeuvre::Log::GetCoreLogger()->error(__VA_ARGS__)
#define OV_CORE_CRITICAL(...) ::Oeuvre::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define OV_TRACE(...)         ::Oeuvre::Log::GetClientLogger()->trace(__VA_ARGS__)
#define OV_INFO(...)          ::Oeuvre::Log::GetClientLogger()->info(__VA_ARGS__)
#define OV_WARN(...)          ::Oeuvre::Log::GetClientLogger()->warn(__VA_ARGS__)
#define OV_ERROR(...)         ::Oeuvre::Log::GetClientLogger()->error(__VA_ARGS__)
#define OV_CRITICAL(...)      ::Oeuvre::Log::GetClientLogger()->critical(__VA_ARGS__)