// logger.hpp

#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include "config.hpp"

namespace utils
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Exception,
        Critical
    };

    namespace
    {
        void print_separator(std::ofstream& _log_file)
        {
            _log_file << "--------------------" << std::endl; // append separator 
        }

        void print_separator_critical(std::ofstream& _log_file)
        {
            _log_file << "====================" << std::endl; // append separator 
        }
    }

    class Logger
    {
        public:
            // Meyer's Singleton
            static Logger& instance()
            {
                static Logger logger;
                return logger;
            }

            // Delete copy/move
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;

            // minimum level to be logged (levels before will be discarded)
            Logger& set_level(LogLevel level)
            {
                _min_level = level;
                return *this;
            }

            Logger& enable_console()
            {
                _console_enabled = true;
                return *this;
            }

            Logger& enable_file()
            {
                _file_enabled = true;
                return *this;
            }

            Logger& disable_console()
            {
                _console_enabled = false;
                return *this;
            }

            Logger& disable_file()
            {
                _file_enabled = false;
                return *this;
            }

            void log(LogLevel level, std::string_view message)
            {
                if (level < _min_level)
                    return;

                std::string formatted = format_message(level, message);

                if (_console_enabled)
                    std::cout << formatted << std::endl;

                if (_file_enabled && _log_file.is_open())
                {
                    _log_file << formatted << std::endl;
                    _log_file.flush();   // force writing to disk immediately
                }
            }

            // Convenience methods
            void debug(std::string_view msg)
            {
                log(LogLevel::Debug, msg);
            }

            void info(std::string_view msg)
            {
                log(LogLevel::Info, msg);
            }

            void warning(std::string_view msg)
            {
                log(LogLevel::Warning, msg);
            }

            void error(std::string_view msg)
            {
                log(LogLevel::Error, msg);
            }

            void exception(std::string_view msg)
            {
                log(LogLevel::Exception, msg);

                if(_file_enabled && _log_file.is_open())
                    print_separator(_log_file); // program terminated via an exception
            }

            void critical(std::string_view msg)
            {
                bool file_active = (_file_enabled && _log_file.is_open());

                if(file_active)
                    print_separator_critical(_log_file);

                log(LogLevel::Critical, msg);

                if(file_active)
                    print_separator_critical(_log_file);
            }

        private:
            Logger()
            {
                _log_file.open(terminal_utils::config::LOG_FILE.data(), std::ios::app);
            }

            ~Logger()
            {
                if (_log_file.is_open())
                {
                    print_separator(_log_file); // program ended normally
                    _log_file.close();
                }
            }

            std::string format_message(LogLevel level, std::string_view message)
            {
                std::ostringstream oss;

                // Timestamp
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");

                // Level
                oss << " [" << level_to_string(level) << "]: ";

                // Message
                oss << message;

                return oss.str();
            }

            std::string_view level_to_string(LogLevel level)
            {
                switch (level)
                {
                    case LogLevel::Debug:     return "DEBUG";
                    case LogLevel::Info:      return "INFO ";
                    case LogLevel::Warning:   return "WARN ";
                    case LogLevel::Error:     return "ERROR";
                    case LogLevel::Exception: return "Exception";
                    case LogLevel::Critical:  return "CRITICAL ";
                    default:                  return "?????";
                }
            }

            LogLevel _min_level = LogLevel::Info;
            bool _console_enabled = false;
            bool _file_enabled = false;
            std::ofstream _log_file;
    };

    // Global convenience macros
    #define LOG_DEBUG(msg)     utils::Logger::instance().debug(msg)
    #define LOG_INFO(msg)      utils::Logger::instance().info(msg)
    #define LOG_WARNING(msg)   utils::Logger::instance().warning(msg)
    #define LOG_ERROR(msg)     utils::Logger::instance().error(msg)
    #define LOG_EXCEPTION(msg) utils::Logger::instance().exception(msg)
    #define LOG_CRITICAL(msg)  utils::Logger::instance().critical(msg)

} // namespace utils
