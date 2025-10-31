#ifndef INTERMEDIARYNODE_H
#define INTERMEDIARYNODE_H

#include <string>
#include <atomic>
#include <thread>
#include "SensorPacket.h"

#include "../../model/structures/sensordata.h"

// Estructura del paquete recibido de Arduino (compatible con ArduinoNode extendido)
#pragma pack(push, 1)
struct SensorPacket {
    uint8_t  msgId;           // 0x42 = SENSOR_DATA
    int16_t  temp_x100;       // temperatura * 100 (network byte order)
    int16_t  hum_x100;        // humedad * 100 (network byte order)
    int16_t  distance_x100;   // distancia * 100 (network byte order)
    int32_t  pressure_pa;     // presión en Pascales (network byte order)
    int16_t  altitude_x100;   // altitud * 100 (network byte order)
};
#pragma pack(pop)

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