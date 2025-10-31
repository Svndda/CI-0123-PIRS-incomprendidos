#pragma once

#include <string>
#include <atomic>
#include <netinet/in.h>

class ArduinoNode {
public:
    // masterIP: IP of the master (server) to send UDP packets to
    // masterPort: port on master
    // serialPath: path to serial device (e.g. /dev/ttyACM0), "stdin" to read from stdin, or "simulate" to generate test data
    ArduinoNode(const std::string& masterIP, int masterPort, const std::string& serialPath = "/dev/ttyACM0");
    ~ArduinoNode();

    // Blocking run loop. Returns when stopped or on fatal error.
    void run();

    // Request stop from another thread
    void stop();

private:
    std::string masterIP_;
    int masterPort_;
    std::string serialPath_;
    std::atomic<bool> running_;
    struct sockaddr_in dst_;

    // internal helpers
    int openSerial(const std::string& path);
    int openUdpSock();
    std::string readLine(int fd);
    bool parseJsonLine(const std::string& line, double& temp, double& hum);
    void sendSensor(double temp, double hum, int usock);
};
