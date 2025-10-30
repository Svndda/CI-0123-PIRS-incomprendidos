#include "LogManager.h"
#include <ctime>


LogManager& LogManager::instance(){
    static LogManager instance;
    return instance;
}

std::string LogManager::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void LogManager::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    LogEntry entry{level, currentTimestamp(), message};
    logs.push_back(entry);
    std::string prefix;
    // print log message
    switch (level) {
        case LogLevel::Debug: prefix = "[DEBUG] "; break;
        case LogLevel::Info: prefix = "[INFO] "; break;
        case LogLevel::Warning: prefix = "[WARNING] "; break;
        case LogLevel::Critical: prefix = "[CRITICAL] "; break;
        case LogLevel::Fatal: prefix = "[FATAL] "; break;
    }
    std::cout << prefix << message << std::endl;
}

std::vector<LogEntry> LogManager::getLogs() const {
    std::lock_guard<std::mutex> lock(mutex);
    return logs;
}

void LogManager::clearLogs() {
    std::lock_guard<std::mutex> lock(mutex);
    logs.clear();
}

void LogManager::debug(const std::string& msg)  { 
    log(LogLevel::Debug, msg); 
}
void LogManager::info(const std::string& msg)   { 
    log(LogLevel::Info, msg); 
}
void LogManager::warning(const std::string& msg) { 
    log(LogLevel::Warning, msg); 
}
void LogManager::critical(const std::string& msg){ 
    log(LogLevel::Critical, msg); 
}
void LogManager::fatal(const std::string& msg)   { 
    log(LogLevel::Fatal, msg); 
}

