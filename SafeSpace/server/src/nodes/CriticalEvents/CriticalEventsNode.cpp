#include "CriticalEventsNode.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <utility>
#include <arpa/inet.h>

CriticalEventsNode::CriticalEventsNode(const std::string& ip, uint16_t port, std::string  outPath)
    : UDPServer(ip, port, 2048), outPath_(std::move(outPath)) {
    // ensure output directory exists (best-effort)
    // do not fail on missing dir; appendLine will create file if needed
    std::cout << "[CriticalEventsNode] listening on UDP port " << port << " -> " << outPath_ << std::endl;
    auto& logger = LogManager::instance();
    logger.enableFileLogging("critical_events_ip_addresses.log");
    logger.ipAddress(ip + std::string(":") + std::to_string(port));
    
}

CriticalEventsNode::~CriticalEventsNode() {
    stop();
}

std::string CriticalEventsNode::makeTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void CriticalEventsNode::appendLine(const std::string& line) {
    std::lock_guard<std::mutex> lk(fileMutex_);
    // open in append mode, create if missing
    std::ofstream f(outPath_, std::ios::app | std::ios::binary);
    if (!f.is_open()) {
        std::cerr << "[CriticalEventsNode] ERROR: cannot open log file: " << outPath_ << std::endl;
        return;
    }
    f << line << "\n";
    f.flush();
}

void CriticalEventsNode::onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len, std::string& out_response) {
    (void)out_response;
    // decode sender
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    uint16_t peerPort = ntohs(peer.sin_port);

    std::string body;
    if (len <= 0) return;
    
    // LogManager protocol: ['L']['O']['G'][level][nodeNameSize][nodeName...][payload...]
    if (len < 5) return; // Minimum size for LogManager protocol
    
    std::string severity = "INFO";
    
    uint8_t levelByte = data[3];
    uint8_t nodeNameSize = data[4];
    
    
    switch (levelByte) {
        case 0: severity = "INFO"; break;
        case 1: severity = "WARN"; break;
        case 2: severity = "ERROR"; break;
        case 3: {
            severity = "IP_ADDRESS";
            
            
        }
        default: severity = "UNKNOWN"; break;
    }
    
    // Skip: 'L'(1) + 'O'(1) + 'G'(1) + level(1) + nodeNameSize(1) + nodeName(nodeNameSize)
    size_t offset = 5 + nodeNameSize;

    // copy rest as UTF-8 text
    body.assign(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(len - offset));

    std::ostringstream line;
    line << makeTimestamp() << "First byte value: " << static_cast<int>(data[1])<< " | " << ipStr << ":" << peerPort << " | " << severity << " | " << body;

    appendLine(line.str());

    // Also print to stdout for convenience
    std::cout << "[CriticalEventsNode] " << line.str() << std::endl;
}
