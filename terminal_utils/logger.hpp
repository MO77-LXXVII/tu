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
    /**
     * @brief severity levels for log messages
     * levels are ordered from least to most severe
     */
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
        /**
         * @brief writes a standard separator line to the log file
         * @param _log_file the output file stream to write to
         */
        void print_separator(std::ofstream& _log_file)
        {
            _log_file << "--------------------" << std::endl;
        }


        /**
         * @brief writes a critical separator line to the log file
         * uses '=' characters to visually distinguish critical log entries
         * @param _log_file the output file stream to write to
         */
        void print_separator_critical(std::ofstream& _log_file)
        {
            _log_file << "====================" << std::endl;
        }
    }


    /**
     * @brief singleton logger supporting console and file output
     *
     * provides leveled logging with optional console and file output
     * configured via a fluent interface. use the `LOG_*` macros for convenience
     *
     * @code
     * utils::Logger::instance()
     *     .set_level(utils::LogLevel::Debug)
     *     .enable_console()
     *     .enable_file();
     *
     * LOG_INFO("Application started");
     * LOG_CRITICAL("Disk full");
     * @endcode
     */
    class Logger
    {
        public:
            /**
             * @brief returns a Meyer's Singleton `Logger` instance
             * @return reference to the global `Logger`
             */
            static Logger& instance()
            {
                static Logger logger;
                return logger;
            }


            // Delete copy/move
            Logger(const Logger&) = delete;
            Logger& operator=(const Logger&) = delete;


            /**
             * @brief sets the minimum log level. messages below this level are discarded
             * @param level The minimum `LogLevel` to log
             * @return reference to this `Logger` for chaining
             */
            Logger& set_level(LogLevel level)
            {
                _min_level = level;
                return *this;
            }


            /**
             * @brief  enables logging to `stdout`
             * @return reference to this `Logger` for chaining
             */
            Logger& enable_console()
            {
                _console_enabled = true;
                return *this;
            }


            /**
             * @brief  enables logging to the configured log file
             * @return reference to this `Logger` for chaining
             */
            Logger& enable_file()
            {
                _file_enabled = true;
                return *this;
            }


            /**
             * @brief  disables logging to `stdout`
             * @return reference to this `Logger` for chaining
             */
            Logger& disable_console()
            {
                _console_enabled = false;
                return *this;
            }


            /**
             * @brief  disables logging to the log file
             * @return reference to this Logger for chaining
             */
            Logger& disable_file()
            {
                _file_enabled = false;
                return *this;
            }


            /**
             * @brief logs a `message` at the given level
             * discards the `message` if level is below the minimum level
             * @param level the severity level of the `message`
             * @param message the `message` to log
             */
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


            /**
             * @brief logs a message at Debug level
             * @param msg the message to log
             */
            // Convenience methods
            void debug(std::string_view msg)
            {
                log(LogLevel::Debug, msg);
            }

            /**
             * @brief logs a message at Info level
             * @param msg the message to log
             */
            void info(std::string_view msg)
            {
                log(LogLevel::Info, msg);
            }

            /**
             * @brief logs a message at Warning level
             * @param msg the message to log
             */
            void warning(std::string_view msg)
            {
                log(LogLevel::Warning, msg);
            }

            /**
             * @brief logs a message at Error level
             * @param msg the message to log
             */
            void error(std::string_view msg)
            {
                log(LogLevel::Error, msg);
            }


            /**
             * @brief logs a message at Exception level and appends a separator
             * the trailing separator indicates the program terminated via an `exception`
             * @param msg The message to log
             */
            void exception(std::string_view msg)
            {
                log(LogLevel::Exception, msg);

                if(_file_enabled && _log_file.is_open())
                    print_separator(_log_file); // program terminated via an exception
            }


            /**
             * @brief logs a message at Critical level, wrapped in critical separators
             * the surrounding `====` separators make critical entries stand out in the log file
             * @param msg The message to log
             */
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
            /** @brief constructs the `Logger` and opens the log file in *append mode* */
            Logger()
            {
                _log_file.open(terminal_utils::config::LOG_FILE.data(), std::ios::app);
            }


            /** @brief the destructor appends a closing separator and closes the log file */
            ~Logger()
            {
                if (_log_file.is_open())
                {
                    print_separator(_log_file); // program ended normally
                    _log_file.close();
                }
            }


            /**
             * @brief formats a log message with a timestamp and level prefix
             * @param level the severity level of the message
             * @param message the raw message text
             * @return the formatted log `std::string`
             */
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


            /**
             * @brief converts a `LogLevel` to its string representation
             * @param level the log level to convert
             * @return `std::string_view` of the level name
             */
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

    /** @brief logs a debug message via the global Logger instance */
    #define LOG_DEBUG(msg)     utils::Logger::instance().debug(msg)

    /** @brief logs a info message via the global Logger instance */
    #define LOG_INFO(msg)      utils::Logger::instance().info(msg)

    /** @brief logs a warning message via the global Logger instance */
    #define LOG_WARNING(msg)   utils::Logger::instance().warning(msg)

    /** @brief logs a error message via the global Logger instance */
    #define LOG_ERROR(msg)     utils::Logger::instance().error(msg)

    /** @brief logs a exception message and appends a closing separator via the global Logger instance */
    #define LOG_EXCEPTION(msg) utils::Logger::instance().exception(msg)

    /** @brief logs a critical message wrapped in critical separators via the global Logger instance */
    #define LOG_CRITICAL(msg)  utils::Logger::instance().critical(msg)

} // namespace utils
