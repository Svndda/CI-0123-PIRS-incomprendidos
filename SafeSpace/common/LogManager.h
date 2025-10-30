#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

// severity levels for logging
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Critical,
    Fatal
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class LogManager {
    public:
        /**
         * @brief Get the singleton instance of LogManager.
         * @brief Get the singleton instance of LogManager.
         *
         * @return LogManager&
         */
        static LogManager& instance();

        /**
         *
         * @return copy of historial logs
         */
        std::vector<LogEntry> getLogs() const;
        /**
         * @brief Clear all stored logs.
         */
        void clearLogs();
        /**
         * @brief Append a message with the provided severity.
         */
        void log(LogLevel level, const std::string& message);
        void debug(const std::string& msg);
        void info(const std::string& msg);
        void warning(const std::string& msg);
        void critical(const std::string& msg);
        void fatal(const std::string& msg);

    private:
    /**
     * @brief Construct a new Qt Log Manager object 
     * 
     */
        LogManager() = default;

        std::string currentTimestamp() const;
        std::vector<LogEntry> logs;
        mutable std::mutex mutex;

       
};


#endif // LOGMANAGER_H
