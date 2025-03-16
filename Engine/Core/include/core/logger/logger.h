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
#include <regex>
#include <core/logger/console_format.h>

namespace destan::core
{
    // Forward declarations
    enum class Logger_Theme;
    struct Theme_Struct;

    /**
     * Log severity levels
     */
    enum class Log_Level
    {
        TRACE,  // Detailed tracing information
        INFO,   // General information messages
        WARN,   // Warning messages
        ERR,    // Error messages
        FATAL,  // Fatal error messages
        NONE    // No logging
    };

    /**
     * The Logger class provides logging capabilities with support for
     * different log levels, formatting, and console styling.
     */
    class Logger
    {
    public:
        /**
         * Get the singleton instance of the logger
         * @return Reference to the logger
         */
        static Logger& Get_Instance()
        {
            static Logger instance;
            return instance;
        }

        /**
         * Start the logger
         * Launches the background thread for processing logs
         */
        void Start();

        /**
         * Stop the logger
         * Stops the background thread and flushes remaining logs
         */
        void Stop();

        /**
        * Set the synchronous mode of the logger. For debugging purposes
        * we need to use this mode to see log results better.
        */
        void Set_Synchronous_Mode(bool synchronous_mode)
        {
			m_synchronous_mode = synchronous_mode;
        }

        /**
		* Set the file output mode of the logger. If this mode is enabled,
		* the logger will write logs to a file in addition to the console.
        */
        void Set_File_Output_Mode(bool file_output_mode)
        {
            m_file_output_mode = file_output_mode;
        }

        /**
         * Log a message with the specified log level
         * @param level The severity level
         * @param message The message to log
         */
        void Log(Log_Level level, const std::string& message);

        /**
         * Log a formatted message with the specified log level
         * Uses C++20 std::format for compile-time format checking
         *
         * @param level The severity level
         * @param fmt The format string
         * @param args Format arguments
         */
        template<typename... Args>
        void Log_Format(Log_Level level, std::format_string<Args...> fmt, Args&&... args)
        {
            std::string formatted_message = std::format(fmt, std::forward<Args>(args)...);
            Log(level, formatted_message);
        }

        /**
         * Apply a predefined theme to the logger
         * @param theme The theme to apply
         */
        static void Apply_Theme(Logger_Theme theme);

        /**
         * Apply a custom theme structure to the logger
         * @param theme_struct The theme structure to apply
         */
        static void Apply_Theme_Struct(const Theme_Struct& theme_struct);

        /**
         * Create a custom theme with specified colors
         * @param trace_color Color for trace messages
         * @param info_color Color for info messages
         * @param warn_color Color for warning messages
         * @param err_color Color for error messages
         * @param fatal_color Color for fatal messages
         * @param timestamp_color Color for timestamp
         * @param message_color Color for message text
         * @return A Theme_Struct with the specified colors
         */
        static Theme_Struct Create_Custom_Theme(
            const char* trace_color,
            const char* info_color,
            const char* warn_color,
            const char* err_color,
            const char* fatal_color,
            const char* timestamp_color,
            const char* message_color);

        /**
         * Get the current theme
         * @return The current theme enumeration
         */
        static Logger_Theme Get_Current_Theme();

        // Static helper methods with format string checking
        template<typename... Args>
        static void Trace(std::format_string<Args...> fmt, Args&&... args)
        {
            Get_Instance().Log_Format(Log_Level::TRACE, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Info(std::format_string<Args...> fmt, Args&&... args)
        {
            Get_Instance().Log_Format(Log_Level::INFO, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Warn(std::format_string<Args...> fmt, Args&&... args)
        {
            Get_Instance().Log_Format(Log_Level::WARN, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Error(std::format_string<Args...> fmt, Args&&... args)
        {
            Get_Instance().Log_Format(Log_Level::ERR, fmt, std::forward<Args>(args)...);
        }

        template<typename... Args>
        static void Fatal(std::format_string<Args...> fmt, Args&&... args)
        {
            Get_Instance().Log_Format(Log_Level::FATAL, fmt, std::forward<Args>(args)...);
        }

        // Plain text versions for runtime strings like string streams
        static void Trace_Text(const std::string& message)
        {
            Get_Instance().Log(Log_Level::TRACE, message);
        }

        static void Info_Text(const std::string& message)
        {
            Get_Instance().Log(Log_Level::INFO, message);
        }

        static void Warn_Text(const std::string& message)
        {
            Get_Instance().Log(Log_Level::WARN, message);
        }

        static void Error_Text(const std::string& message)
        {
            Get_Instance().Log(Log_Level::ERR, message);
        }

        static void Fatal_Text(const std::string& message)
        {
            Get_Instance().Log(Log_Level::FATAL, message);
        }

    private:
        Logger();
        ~Logger();

        void Process_Logs();

        std::queue<std::pair<Log_Level, std::string>> m_log_queue;
        std::mutex m_mutex;
        std::condition_variable m_condition_variable;
        std::thread m_log_thread;
        bool m_synchronous_mode = false;
        bool m_file_output_mode = false;
        bool m_running;
    };
}

// Macros for logging with format strings
#define DESTAN_LOG_TRACE(...) destan::core::Logger::Trace(__VA_ARGS__)
#define DESTAN_LOG_INFO(...) destan::core::Logger::Info(__VA_ARGS__)
#define DESTAN_LOG_WARN(...) destan::core::Logger::Warn(__VA_ARGS__)
#define DESTAN_LOG_ERROR(...) destan::core::Logger::Error(__VA_ARGS__)
#define DESTAN_LOG_FATAL(...) destan::core::Logger::Fatal(__VA_ARGS__)

// Macros for logging plain text (e.g., string streams)
#define DESTAN_LOG_TRACE_TEXT(message) destan::core::Logger::Trace_Text(message)
#define DESTAN_LOG_INFO_TEXT(message) destan::core::Logger::Info_Text(message)
#define DESTAN_LOG_WARN_TEXT(message) destan::core::Logger::Warn_Text(message)
#define DESTAN_LOG_ERROR_TEXT(message) destan::core::Logger::Error_Text(message)
#define DESTAN_LOG_FATAL_TEXT(message) destan::core::Logger::Fatal_Text(message)

// Macros for assertions, tied to logging that's why they are defined here
#ifdef DESTAN_ENABLE_ASSERTS
#define DESTAN_ASSERT(x, ...) { if(!(x)) { DESTAN_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
#define DESTAN_CORE_ASSERT(x, ...) { if(!(x)) { DESTAN_LOG_ERROR("Assertion Failed: {0}", __VA_ARGS__); DESTAN_DEBUGBREAK(); } }
#else
#define DESTAN_ASSERT(x, ...)
#define DESTAN_CORE_ASSERT(x, ...)
#endif