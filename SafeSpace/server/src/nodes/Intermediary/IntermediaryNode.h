#ifndef INTERMEDIARYNODE_H
#define INTERMEDIARYNODE_H

#include <string>
#include <atomic>
#include <thread>
#include "SensorPacket.h"

#include "../../model/structures/sensordata.h"

// Estructura del paquete recibido de Arduino 
// #pragma pack(push, 1)
// struct SensorPacket {
//     uint8_t  msgId;           // 0x42 = SENSOR_DATA
//     float    distance;        // distancia en cm
//     float    temperature;     // temperatura en °C
//     float    pressure;        // presión en Pa
//     float    altitude;        // altitud en metros
//     float    sealevelPressure; // presión a nivel del mar en Pa
//     float    realAltitude;    // altitud real en metros
// };
// #pragma pack(pop)

class IntermediaryNode {
private:
    int listen_port_;
    std::string master_ip_;
    int master_port_;
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