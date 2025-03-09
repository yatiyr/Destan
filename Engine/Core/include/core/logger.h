#pragma once
#include <core/defines.h>
#include <queue>
#include <string>
#include <string_view>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <format>
#include <algorithm>
#include <utility>

namespace destan::core
{
	enum class Log_Level
	{
		TRACE,
		INFO,
		WARN,
		ERR,
		FATAL,
		NONE
	};

	class Logger
	{
	public:
		static Logger& Get_Instance()
		{
			static Logger instance;
			return instance;
		}

		void Start();

		void Stop();

		void Log(Log_Level level, const std::string& message);

		template<typename... Args>
		void Log_Format(Log_Level level, const std::string_view format_string, Args&&... args)
		{
			destan::String formatted_message = std::format(format_string, std::forward<Args>(args)...);
			Log(level, formatted_message);)
		}

		template<typename... Args>
		static void Trace(const std::string_view format_string, Args&&... args)
		{
			Get_Instance().Log_Format(Log_Level::TRACE, format_string, std::forward<Args>(args)...);
		}

		template<typename... Args>
		static void Info(const std::string_view format_string, Args&&... args)
		{
			Get_Instance().Log_Format(Log_Level::INFO, format_string, std::forward<Args>(args)...);
		}

		template<typename... Args>
		static void Warn(const std::string_view format_string, Args&&... args)
		{
			Get_Instance().Log_Format(Log_Level::WARN, format_string, std::forward<Args>(args)...);
		}

		template<typename... Args>
		static void Error(const std::string_view format_string, Args&&... args)
		{
			Get_Instance().Log_Format(Log_Level::ERR, format_string, std::forward<Args>(args)...);
		}

		template<typename... Args>
		static void Fatal(const std::string_view format_string, Args&&... args)
		{
			Get_Instance().Log_Format(Log_Level::FATAL, format_string, Args&&... args);
		}

	private:
		Logger();
		~Logger();

		void Process_Logs();

		std::queue<std::pair<Log_Level, std::string>> m_log_queue;
		std::mutex m_mutex;
		std::condition_variable m_condition_variable;
		std::thread m_log_thread;
		bool m_running;
	};
}


// Macros for logging
#define DESTAN_LOG_TRACE(...) destan::core::Logger::Trace(__VA_ARGS__)
#define DESTAN_LOG_INFO(...) destan::core::Logger::Info(__VA_ARGS__)
#define DESTAN_LOG_WARN(...) destan::core::Logger::Warn(__VA_ARGS__)
#define DESTAN_LOG_ERROR(...) destan::core::Logger::Error(__VA_ARGS__)
#define DESTAN_LOG_FATAL(...) destan::core::Logger::Fatal(__VA_ARGS__)


// Macros for assertions, tied to logging that's why they are defined here
#ifdef DESTAN_ENABLE_ASSERTS
	#define DESTAN_ASSERT(x, ...) { if(!(x)) { DESTAN_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
	#define DESTAN_CORE_ASSERT(x, ...) { if(!(x)) { DESTAN_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
#else
	#define DESTAN_ASSERT(x, ...)
	#define DESTAN_CORE_ASSERT(x, ...)
#endif