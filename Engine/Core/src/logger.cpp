#include <core/destan_pch.h>
#include <core/logger.h>

namespace destan::core
{

	static std::string Get_Current_Time()
	{
		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::ostringstream ss;
		ss << std::put_time(std::localtime(&in_time_t), "[%Y-%m-%d %H:%M:%S]");
		return ss.str();
	}

	static const char* Get_Log_Level_Color(Log_Level level)
	{
		switch (level)
		{
			case Log_Level::TRACE:
				return "\033[37m";
			case Log_Level::INFO:
				return "\033[32m";
			case Log_Level::WARN:
				return "\033[33m";
			case Log_Level::ERR:
				return "\033[31m";
			case Log_Level::FATAL:
				return "\033[41m";
			default:
				return "\033[37m";
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
		m_running = true;
		m_log_thread = std::thread(&Logger::Process_Logs, this);
	}

	void Logger::Stop()
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
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
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_log_queue.emplace(std::pair { level, message });
		}
		m_condition_variable.notify_one();
	}

	void Logger::Process_Logs()
	{
		std::ofstream log_file("destan.log", std::ios::app);

		while (m_running)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_condition_variable.wait(lock, [this] { return !m_log_queue.empty() || !m_running; });


			while (!m_log_queue.empty())
			{
				auto [level, message] = m_log_queue.front();
				m_log_queue.pop();
				lock.unlock();

				// Get date-time format
				std::string timestamp = Get_Current_Time();

				// Log level and color formatting
				const char* color = Get_Log_Level_Color(level);
				std::string log_level_string;
				switch(level)
				{
					case Log_Level::TRACE:
						log_level_string = "[TRACE]";
						break;
					case Log_Level::INFO:
						log_level_string = "[INFO]";
						break;
					case Log_Level::WARN:
						log_level_string = "[WARN]";
						break;
					case Log_Level::ERR:
						log_level_string = "[ERR]";
						break;
					case Log_Level::FATAL:
						log_level_string = "[FATAL]";
						break;
					default:
						log_level_string = "[NONE]";
						break;
				}

				// Write to console
				std::cout << timestamp << " " << color << log_level_string << " " << message << "\033[0m" << std::endl;

				// Write to file
				if (log_file.is_open())
				{
					log_file << timestamp << " " << log_level_string << " " << message << std::endl;
				}

				lock.lock();
			}
		}

		log_file.close();
	}
} // namespace destan::core