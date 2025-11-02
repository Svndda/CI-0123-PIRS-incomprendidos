#ifndef INTERMEDIARYNODE_H
#define INTERMEDIARYNODE_H

#include <string>
#include <atomic>
#include <thread>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SensorPacket.h"

#include "../../model/structures/sensordata.h"

class IntermediaryNode {
private:
    int listen_port_;
    std::string master_ip_;
    int master_port_;
    int master_sock_;          ///< Socket usado para enviar datos al Master
    sockaddr_in master_addr_;  ///< Dirección del Master
    int listen_sock_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
    
    static const int BUFFER_SIZE = 1024;

    // Métodos privados
    bool createUdpSocket();
    void setupMasterConnection();
    void processSensorPacket(const SensorPacket& packet);
    void workerThread();

public:
    IntermediaryNode(int listen_port, const std::string& master_ip, int master_port);
    ~IntermediaryNode();
    
    bool start();
    void stop();
    bool isRunning() const { return running_; }
};

#endif // INTERMEDIARYNODE_H