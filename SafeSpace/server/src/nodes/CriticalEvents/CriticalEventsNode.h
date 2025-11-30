#pragma once

#include "../interfaces/UDPServer.h"
#include <string>
#include <mutex>
#include <vector>
#include <thread>
#include <atomic>

class CriticalEventsNode : public UDPServer {
public:
    // Listen on given UDP port and append received events to a host file
    explicit CriticalEventsNode(const std::string& ip, uint16_t port, std::string  outPath = "data/events.log");
    ~CriticalEventsNode() override;

    void onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len, std::string& out_response) override;
    
    
private:
    std::string outPath_;
    std::mutex fileMutex_;

    std::vector<std::string> batchBuffer;
    std::mutex batchMutex;
    std::thread batchThread; 
    std::atomic<bool> batchRunning_{false};
    std::string masterIp;
    uint16_t masterPort;

    void appendLine(const std::string& line);
    std::string makeTimestamp();
    void batchWorkerLoop();                  
    void startBatchForwarder(std::string ip, uint16_t port);
    void stopBatchForwarder();
};
