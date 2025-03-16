#include <core/destan_pch.h>
#include <core/logger/logger.h>
#include <core/logger/console_format.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <regex>

namespace destan::core
{
    // Static members for theme
    static Theme_Struct s_current_theme = DEFAULT_THEME;
    static Logger_Theme s_current_theme_enum = Logger_Theme::DEFAULT;

    // Helper function to get current time string
    static std::string Get_Current_Time()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

        std::ostringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S]");
        return ss.str();
    }

    // Helper function to get the color for a log level based on current theme
    static const char* Get_Log_Level_Color(Log_Level level)
    {
        switch (level)
        {
        case Log_Level::TRACE:
            return s_current_theme.trace_color;
        case Log_Level::INFO:
            return s_current_theme.info_color;
        case Log_Level::WARN:
            return s_current_theme.warn_color;
        case Log_Level::ERR:
            return s_current_theme.err_color;
        case Log_Level::FATAL:
            return s_current_theme.fatal_color;
        default:
            return s_current_theme.message_color;
        }
    }

    // Helper function to get the string representation of a log level
    static std::string Get_Log_Level_String(Log_Level level)
    {
        switch (level)
        {
        case Log_Level::TRACE:
            return "[TRACE]";
        case Log_Level::INFO:
            return "[INFO]";
        case Log_Level::WARN:
            return "[WARN]";
        case Log_Level::ERR:
            return "[ERR]";
        case Log_Level::FATAL:
            return "[FATAL]";
        default:
            return "[NONE]";
        }
    }

    Logger::Logger() : m_running(false)
    {
        // Initialize the logger
    }

    Logger::~Logger()
    {
        Stop();
    }

    void Logger::Start()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running)
            {
                m_running = true;
                m_log_thread = std::thread(&Logger::Process_Logs, this);
            }
        } // lock is released here

        // We can't use DESTAN_LOG_INFO here directly as it would be recursive
        std::string msg = "Logger started";
        Log(Log_Level::INFO, msg);

        // Give the thread a moment to initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void Logger::Stop()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running)
                return;

            m_running = false;
        }

        m_condition_variable.notify_one();

        if (m_log_thread.joinable())
        {
            m_log_thread.join();
        }
    }

    void Logger::Log(Log_Level level, const std::string& message)
    {
        if (m_synchronous_mode)
        {
            std::ofstream log_file;
            if (m_file_output_mode)
            {
                log_file = std::ofstream("destan.log", std::ios::app);
            }

            // Get current time
            std::string timestamp = Get_Current_Time();

            // Get log level colors and strings
            const char* level_color = Get_Log_Level_Color(level);
            std::string level_string = Get_Log_Level_String(level);

            // Format for console with colors
            std::cout << s_current_theme.timestamp_color << timestamp << console_format::RESET << " "
                << level_color << level_string << console_format::RESET << " "
                << s_current_theme.message_color << message << console_format::RESET << std::endl;

            // Write to file (without ANSI color codes)
            if (m_file_output_mode)
            {
                if (log_file.is_open())
                {
                    // Use regex to strip ANSI color codes for file output
                    std::string clean_message = message;
                    std::regex ansi_regex("\\x1B\\[[0-9;]*[mK]");
                    clean_message = std::regex_replace(clean_message, ansi_regex, "");

                    log_file << timestamp << " " << level_string << " " << clean_message << std::endl;
                }
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_log_queue.emplace(level, message);
        }
        m_condition_variable.notify_one();
    }

    void Logger::Process_Logs()
    {
        std::ofstream log_file;
        if (m_file_output_mode)
        {
            log_file = std::ofstream("destan.log", std::ios::app);
        }

        while (m_running)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition_variable.wait(lock, [this] { return !m_log_queue.empty() || !m_running; });

            if (!m_running && m_log_queue.empty())
                break;

            while (!m_log_queue.empty())
            {
                auto [level, message] = m_log_queue.front();
                m_log_queue.pop();
                lock.unlock();

                // Get current time
                std::string timestamp = Get_Current_Time();

                // Get log level colors and strings
                const char* level_color = Get_Log_Level_Color(level);
                std::string level_string = Get_Log_Level_String(level);

                // Format for console with colors
                std::cout << s_current_theme.timestamp_color << timestamp << console_format::RESET << " "
                    << level_color << level_string << console_format::RESET << " "
                    << s_current_theme.message_color << message << console_format::RESET << std::endl;

                // Write to file (without ANSI color codes)
                if (m_file_output_mode)
                {
                    if (log_file.is_open())
                    {
                        // Use regex to strip ANSI color codes for file output
                        std::string clean_message = message;
                        std::regex ansi_regex("\\x1B\\[[0-9;]*[mK]");
                        clean_message = std::regex_replace(clean_message, ansi_regex, "");

                        log_file << timestamp << " " << level_string << " " << clean_message << std::endl;
                    }
                }

                lock.lock();
            }
        }

        if (log_file.is_open())
        {
            log_file.close();
        }
    }

    void Logger::Apply_Theme(Logger_Theme theme)
    {
        switch (theme)
        {
        case Logger_Theme::DEFAULT:
            s_current_theme = DEFAULT_THEME;
            break;
        case Logger_Theme::DARK:
            s_current_theme = DARK_THEME;
            break;
        case Logger_Theme::LIGHT:
            s_current_theme = LIGHT_THEME;
            break;
        case Logger_Theme::VIBRANT:
            s_current_theme = VIBRANT_THEME;
            break;
        case Logger_Theme::MONOCHROME:
            s_current_theme = MONOCHROME_THEME;
            break;
        case Logger_Theme::PASTEL:
            s_current_theme = PASTEL_THEME;
            break;
        default:
            s_current_theme = DEFAULT_THEME;
            break;
        }

        s_current_theme_enum = theme;

        // Log the theme change using string concatenation
        std::string theme_name;
        switch (theme) {
        case Logger_Theme::DEFAULT: theme_name = "DEFAULT"; break;
        case Logger_Theme::DARK: theme_name = "DARK"; break;
        case Logger_Theme::LIGHT: theme_name = "LIGHT"; break;
        case Logger_Theme::VIBRANT: theme_name = "VIBRANT"; break;
        case Logger_Theme::MONOCHROME: theme_name = "MONOCHROME"; break;
        case Logger_Theme::PASTEL: theme_name = "PASTEL"; break;
        default: theme_name = "UNKNOWN"; break;
        }

        // Avoid using format string since it's causing issues
        std::string msg = "Applied theme: " + theme_name;
        Logger::Get_Instance().Log(Log_Level::INFO, msg);
    }

    void Logger::Apply_Theme_Struct(const Theme_Struct& theme_struct)
    {
        s_current_theme = theme_struct;
        s_current_theme_enum = Logger_Theme::CUSTOM;
        Logger::Get_Instance().Log(Log_Level::INFO, "Applied custom theme");
    }

    Theme_Struct Logger::Create_Custom_Theme(
        const char* trace_color,
        const char* info_color,
        const char* warn_color,
        const char* err_color,
        const char* fatal_color,
        const char* timestamp_color,
        const char* message_color)
    {
        Theme_Struct theme;
        theme.trace_color = trace_color ? trace_color : console_format::FG_WHITE;
        theme.info_color = info_color ? info_color : console_format::FG_GREEN;
        theme.warn_color = warn_color ? warn_color : console_format::FG_YELLOW;
        theme.err_color = err_color ? err_color : console_format::FG_RED;
        theme.fatal_color = fatal_color ? fatal_color : console_format::BG_RED;
        theme.timestamp_color = timestamp_color ? timestamp_color : console_format::FG_CYAN;
        theme.message_color = message_color ? message_color : console_format::RESET;

        return theme;
    }

    Logger_Theme Logger::Get_Current_Theme()
    {
        return s_current_theme_enum;
    }
}