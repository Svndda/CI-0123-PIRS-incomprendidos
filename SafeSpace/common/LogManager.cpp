#include "LogManager.h"
#include <ctime>
#include <arpa/inet.h> //
#include <sys/socket.h> 
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <algorithm>

LogManager& LogManager::instance(){
    static LogManager instance;
    return instance;
}


LogManager::~LogManager() {
    disableRemote();
    disableFileLogging();
}

std::string LogManager::currentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void LogManager::log(LogLevel level, const std::string& message) {
    const std::string timestamp = currentTimestamp();
    {
        std::lock_guard<std::mutex> lock(mutex);
        logs.push_back(LogEntry{level, timestamp, message});
    }
    std::string prefix;
    
    switch (level) {
        case LogLevel::Info: prefix = "[INFO] "; break;
        case LogLevel::Warning: prefix = "[WARNING] "; break;
        case LogLevel::Error: prefix = "[ERROR] "; break;
        case LogLevel::Ip_Address: prefix = "[IP_ADDRESS] "; break;
    }
    std::cout << prefix << message << std::endl;
    
    // Write to file if file logging is enabled
    {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        if (fileLoggingEnabled && logFile.is_open()) {
            logFile << timestamp << " " << prefix << message << std::endl;
            logFile.flush(); // Ensure immediate write for security logs
        }
    }
    
    sendToRemote(level, timestamp, message);
}

std::vector<LogEntry> LogManager::getLogs() const {
    std::lock_guard<std::mutex> lock(mutex);
    return logs;
}

void LogManager::clearLogs() {
    std::lock_guard<std::mutex> lock(mutex);
    logs.clear();
}


void LogManager::info(const std::string& msg)   { 
    log(LogLevel::Info, msg); 
}
void LogManager::warning(const std::string& msg) { 
    log(LogLevel::Warning, msg); 
}
void LogManager::error(const std::string& msg){ 
    log(LogLevel::Error, msg); 
}
void LogManager::ipAddress(const std::string& msg){ 
    log(LogLevel::Ip_Address, msg); 
}


void LogManager::configureRemote(const std::string& ip, uint16_t port, const std::string& nodeName) {
    std::lock_guard<std::mutex> lock(remoteMutex);
    if (remoteSocket < 0) {
        remoteSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (remoteSocket < 0) {
            std::cerr << "[LogManager] Failed to create remote socket: " << std::strerror(errno) << std::endl;
            remoteConfigured = false;
            return;
        }
    }
    std::memset(&remoteAddr, 0, sizeof(remoteAddr));
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(port);
    if (::inet_aton(ip.c_str(), &remoteAddr.sin_addr) == 0) {
        std::cerr << "[LogManager] Invalid remote IP: " << ip << std::endl;
        remoteConfigured = false;
        return;
    }
    nodeIdentifier = sanitize(nodeName.empty() ? "node" : nodeName);
    remoteConfigured = true;
}
void LogManager::disableRemote() {
    std::lock_guard<std::mutex> lock(remoteMutex);
    if (remoteSocket >= 0) {
        ::close(remoteSocket);
        remoteSocket = -1;
    }
    remoteConfigured = false;
}
void LogManager::sendNodeName(const std::string& nodeName) {
    std::lock_guard<std::mutex> lock(remoteMutex);
    nodeIdentifier = sanitize(nodeName.empty() ? "node" : nodeName);
}
void LogManager::sendToRemote(LogLevel level, const std::string& timestamp, const std::string& message) {
    int socketCopy = -1;
    sockaddr_in addrCopy{};
    std::string nodeLabel;
    {
        std::lock_guard<std::mutex> lock(remoteMutex);
        if (!remoteConfigured || remoteSocket < 0) {
            return;
        }
        socketCopy = remoteSocket;
        addrCopy = remoteAddr;
        nodeLabel = nodeIdentifier;
    }
    if (nodeLabel.size() > 255) {
        nodeLabel.resize(255);
    }
    std::string sanitizedMessage = sanitize(message);
    std::string payload = timestamp.empty() ? sanitizedMessage : (timestamp + " | " + sanitizedMessage);
    std::vector<uint8_t> buffer;
    buffer.reserve(5 + nodeLabel.size() + payload.size());
    buffer.push_back('L');
    buffer.push_back('O');
    buffer.push_back('G');
    buffer.push_back(static_cast<uint8_t>(level));
    buffer.push_back(static_cast<uint8_t>(nodeLabel.size()));
    buffer.insert(buffer.end(), nodeLabel.begin(), nodeLabel.end());
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    ssize_t sent = ::sendto(socketCopy,
                            reinterpret_cast<const char*>(buffer.data()),
                            buffer.size(),
                            0,
                            reinterpret_cast<const sockaddr*>(&addrCopy),
                            sizeof(addrCopy));
    if (sent < 0) {
        std::cerr << "[LogManager] Failed to send remote log: " << std::strerror(errno) << std::endl;
    }
}
std::string LogManager::sanitize(const std::string& text) const {
    std::string cleaned = text;
    std::replace(cleaned.begin(), cleaned.end(), '\n', ' ');
    std::replace(cleaned.begin(), cleaned.end(), '\r', ' ');
    return cleaned;
}

void LogManager::enableFileLogging(const std::string& filename) {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filename, std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        fileLoggingEnabled = true;
        // Write header to log file
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        logFile << "\n=== LOGGING SESSION STARTED: " << ss.str() << " ===" << std::endl;
        logFile.flush();
        std::cout << "[LogManager] File logging enabled: " << filename << std::endl;
    } else {
        fileLoggingEnabled = false;
        std::cerr << "[LogManager] Failed to open log file: " << filename << std::endl;
    }
}

void LogManager::disableFileLogging() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        logFile << "=== LOGGING SESSION ENDED: " << ss.str() << " ===" << std::endl;
        logFile.close();
    }
    fileLoggingEnabled = false;
}