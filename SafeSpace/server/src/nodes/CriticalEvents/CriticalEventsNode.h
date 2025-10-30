#pragma once

#include "../interfaces/UDPServer.h"
#include <string>
#include <mutex>

class CriticalEventsNode : public UDPServer {
public:
    // Listen on given UDP port and append received events to a host file
    explicit CriticalEventsNode(uint16_t port, const std::string& outPath = "data/events.log");
    ~CriticalEventsNode() override;

    void onReceive(const sockaddr_in& peer, const uint8_t* data, ssize_t len, std::string& out_response) override;

private:
    std::string outPath_;
    std::mutex fileMutex_;

    void appendLine(const std::string& line);
    std::string makeTimestamp();
};
