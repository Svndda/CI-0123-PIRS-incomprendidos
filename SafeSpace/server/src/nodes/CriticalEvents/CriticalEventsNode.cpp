#include "CriticalEventsNode.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <arpa/inet.h>

CriticalEventsNode::CriticalEventsNode(uint16_t port, const std::string& outPath)
    : UDPServer(port, 2048), outPath_(outPath) {
    // ensure output directory exists (best-effort)
    // do not fail on missing dir; appendLine will create file if needed
    std::cout << "[CriticalEventsNode] listening on UDP port " << port << " -> " << outPath_ << std::endl;
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

    // If the first byte is a known severity, interpret it; otherwise treat whole datagram as text
    std::string severity = "INFO";
    size_t offset = 0;
    if (len >= 1) {
        uint8_t first = data[0];
        if (first <= 2 && len >= 2) {
            // format: [severity(0=INFO,1=WARN,2=ERROR)] payload...
            switch (first) {
                case 0: severity = "INFO"; break;
                case 1: severity = "WARN"; break;
                case 2: severity = "ERROR"; break;
                default: severity = "INFO"; break;
            }
            offset = 1;
        }
    }

    // copy rest as UTF-8 text
    body.assign(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(len - offset));

    std::ostringstream line;
    line << makeTimestamp() << " | " << ipStr << ":" << peerPort << " | " << severity << " | " << body;

    appendLine(line.str());

    // Also print to stdout for convenience
    std::cout << "[CriticalEventsNode] " << line.str() << std::endl;
}
