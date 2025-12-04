#include "CriticalEventsNode.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <utility>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

CriticalEventsNode::CriticalEventsNode(const std::string& ip, uint16_t port, std::string  outPath)
    : UDPServer(ip, port, 2048), outPath_(std::move(outPath)) {
    // ensure output directory exists (best-effort)
    // do not fail on missing dir; appendLine will create file if needed
    std::cout << "[CriticalEventsNode] listening on UDP port " << port << " -> " << outPath_ << std::endl;
    auto& logger = LogManager::instance();
    logger.enableFileLogging("critical_events_ip_addresses.log");
    logger.ipAddress(ip + std::string(":") + std::to_string(port));

    startBatchForwarder();
    
}

CriticalEventsNode::~CriticalEventsNode() {
    stopBatchForwarder();
    stop();
    auto& logger = LogManager::instance();
    logger.info("CriticalEventsNode shutting down - Security logging ended");
    logger.disableFileLogging();
    batchRunning_.store(false);
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
        case 3: severity = "IP_ADDRESS"; break;      
        default: severity = "UNKNOWN"; break;
    }
    
    // Skip: 'L'(1) + 'O'(1) + 'G'(1) + level(1) + nodeNameSize(1) + nodeName(nodeNameSize)
    size_t offset = 5 + nodeNameSize;

    // copy rest as UTF-8 text
    body.assign(reinterpret_cast<const char*>(data + offset), static_cast<size_t>(len - offset));

    std::ostringstream line;
    line << makeTimestamp() << "First byte value: " << static_cast<int>(data[1])<< " | " << ipStr << ":" << peerPort << " | " << severity << " | " << body;
    
    std::lock_guard<std::mutex> lk(batchMutex);
    batchBuffer.push_back(line.str());
    
    appendLine(line.str());

    // Also print to stdout for convenience
    std::cout << "[CriticalEventsNode] " << line.str() << std::endl;
}

void CriticalEventsNode::startBatchForwarder(){
    if (batchRunning_.exchange(true)) {
        return; // already running
    }
    batchThread = std::thread(&CriticalEventsNode::batchWorkerLoop, this);
}

void CriticalEventsNode::stopBatchForwarder(){
    batchRunning_.store(false);
    if (batchThread.joinable()) {
        batchThread.join();
    }
}

void CriticalEventsNode::batchWorkerLoop(){
    using namespace std::chrono_literals;
    int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "[CriticalEventsNode] ERROR: cannot create UDP socket for batch forwarding" << std::endl;
        auto& logger = LogManager::instance();
        logger.error("CriticalEventsNode] ERROR: cannot create UDP socket for batch forwarding");
        return;
    }

    bool warnedAboutConfig = false;

    while (this->batchRunning_.load()) {
        std::this_thread::sleep_for(std::chrono::minutes(1));

        if (!masterConfigured_.load()) {
            if (!warnedAboutConfig) {
                LogManager::instance().warning("CriticalEventsNode batch forwarder awaiting master configuration");
                warnedAboutConfig = true;
            }
            continue;
        }
        warnedAboutConfig = false;
        // swap buffer
        std::vector<std::string> snapshot;
        std::string targetIp;
        uint16_t targetPort = 0;
        {
            std::lock_guard<std::mutex> lk(batchMutex);
            if (batchBuffer.empty()) {
                continue; // nothing to send
            }
            snapshot.swap(batchBuffer);
            targetIp = masterIp_;
            targetPort = masterPort_;
        }

        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(targetPort);
        if (::inet_aton(targetIp.c_str(), &dest.sin_addr) == 0) {
            LogManager::instance().error("CriticalEventsNode batch forwarder invalid master IP: " + targetIp);
            {
                std::lock_guard<std::mutex> lk(batchMutex);
                batchBuffer.insert(batchBuffer.begin(), snapshot.begin(), snapshot.end());
            }
            continue;
        }
        std::ostringstream batchData;
        batchData << "LOG_BATCH\n";
        for (const auto& line : snapshot) {
            batchData << line << "\n";
        }
        std::string batchStr = batchData.str();
        ssize_t sent = ::sendto(sock, batchStr.data(), batchStr.size(), 0,
                                reinterpret_cast<sockaddr*>(&dest), sizeof(dest));

        if (sent < 0) {
            auto& logger = LogManager::instance();
            const std::string err = std::strerror(errno);
            std::cerr << "[CriticalEventsNode] ERROR: failed to send batch UDP packet: " << err << std::endl;
            logger.error("CriticalEventsNode batch forward send failure: " + err);
        }
        
    }   
    ::close(sock);
}

void CriticalEventsNode::configureMasterForwarding(const std::string& masterIp, uint16_t masterPort) {
    std::lock_guard<std::mutex> lk(batchMutex);
    masterIp_ = masterIp;
    masterPort_ = masterPort;
    this->masterConfigured_.store(true);
    LogManager::instance().info("CriticalEventsNode: Configured master forwarding to " + masterIp + ":" + std::to_string(masterPort));
}