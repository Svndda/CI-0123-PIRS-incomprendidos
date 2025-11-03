#ifndef LOGMANAGER_H
#define LOGMANAGER_H
#include <string>
#include <vector>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <netinet/in.h>
// severity levels for logging
enum class LogLevel {
    Info,
    Warning,
    Error,
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
         * @brief Destructor for LogManager
         */
        ~LogManager();

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
        void info(const std::string& msg);
        void warning(const std::string& msg);
        void error(const std::string& msg);
        /**
         * @brief Configure remote logging.
         * 
         * @param ip 
         * @param port 
         * @param nodeName 
         */
        void configureRemote(const std::string& ip, uint16_t port, const std::string& nodeName);
        /**
         * @brief  Disable remote logging.
         * 
         * 
         */
        void disableRemote();
        /**
         * @brief Send the local node's name to the remote server.
         * 
         * @param nodeName 
         */
        void sendNodeName(const std::string& nodeName);

    private:
    /**
     * @brief Construct a new Qt Log Manager object 
     * 
     */
        LogManager() = default;
        /**
         * @brief Construct a new Log Manager object    
         * 
         */
        LogManager(const LogManager&) = delete;
         /**
          * @brief Assignment operator for LogManager.  
          * 
          * @return LogManager& 
          */
        LogManager& operator=(const LogManager&) = delete;
        /**
         * @brief Get the current timestamp.
         * 
         * @return std::string 
         */
        std::string currentTimestamp() const;
        /**
         * @brief Send a log message to the remote server.
         * 
         * @param level 
         * @param timestamp 
         * @param message 
         */
        void sendToRemote(LogLevel level, const std::string& timestamp, const std::string& message);
        /**
         * @brief Sanitize a string for safe logging.
         * 
         * @param text 
         * @return std::string 
         */
        std::string sanitize(const std::string& text) const;
        /**
         * @brief   Stored log entries.
         * 
         */
        std::vector<LogEntry> logs;
        /**
         * @brief Mutex for protecting log access.
         * 
         */
        mutable std::mutex mutex;
        /**
         * @brief Mutex for protecting remote log access.
         * 
         */
        mutable std::mutex remoteMutex;
        /**
         * @brief Socket for remote logging.
         * 
         */
        int remoteSocket{-1};
        /**
         * @brief Address for remote logging.
         * 
         */
        sockaddr_in remoteAddr{};
        /**
         * @brief Flag indicating whether remote logging is configured.
         * 
         */
        bool remoteConfigured{false};
        /**
         * @brief Identifier for the local node.
         * 
         */
        std::string nodeIdentifier{"node"};
           
};


#endif // LOGMANAGER_H
