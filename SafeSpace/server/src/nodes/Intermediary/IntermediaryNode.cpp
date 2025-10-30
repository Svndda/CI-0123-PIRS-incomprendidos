#include "IntermediaryNode.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <chrono>

IntermediaryNode::IntermediaryNode(int listen_port, const std::string& master_ip, int master_port)
    : listen_port_(listen_port), master_ip_(master_ip), master_port_(master_port),
      listen_sock_(-1), running_(false) {
    
    std::cout << "[IntermediaryNode] Configurado - Puerto: " << listen_port 
              << ", Master: " << master_ip << ":" << master_port << std::endl;
}

IntermediaryNode::~IntermediaryNode() {
    stop();
}

bool IntermediaryNode::createUdpSocket() {
    listen_sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sock_ < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(listen_port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_sock_, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
        perror("bind");
        close(listen_sock_);
        listen_sock_ = -1;
        return false;
    }

    std::cout << "[IntermediaryNode] Escuchando en puerto " << listen_port_ << std::endl;
    return true;
}

void IntermediaryNode::setupMasterConnection() {
    std::cout << "[IntermediaryNode] Configurado para enviar a Master_node en " 
              << master_ip_ << ":" << master_port_ << std::endl;
}

void IntermediaryNode::processSensorPacket(const SensorPacket& packet) {
    // Los datos ya vienen en formato float, no necesitamos conversión
    std::cout << "[IntermediaryNode] Datos recibidos:" << std::endl;
    std::cout << "  - Distancia: " << packet.distance << " cm" << std::endl;
    std::cout << "  - Temperatura: " << packet.temperature << " °C" << std::endl;
    std::cout << "  - Presión: " << packet.pressure << " Pa" << std::endl;
    std::cout << "  - Altitud: " << packet.altitude << " m" << std::endl;
    std::cout << "  - Presión nivel del mar: " << packet.sealevelPressure << " Pa" << std::endl;
    std::cout << "  - Altitud real: " << packet.realAltitude << " m" << std::endl;

    // Crear objeto SensorData con los nuevos campos
    SensorData sensorData(
        packet.distance,      // distance
        packet.temperature,   // temperature  
        packet.pressure,      // pressure
        packet.altitude,      // altitude
        packet.sealevelPressure, // sealevelPressure
        packet.realAltitude   // realAltitude
    );

    std::cout << "[IntermediaryNode] Preparado para enviar SensorData al Master_node:" << std::endl;
    std::cout << "[IntermediaryNode] - Distancia: " << sensorData.distance << " cm" << std::endl;
    std::cout << "[IntermediaryNode] - Temperatura: " << sensorData.temperature << " °C" << std::endl;
    std::cout << "[IntermediaryNode] - Presión: " << sensorData.pressure << " Pa" << std::endl;
    
    // TODO: Cuando el Master_node esté listo, descomentar:
    // send(master_sock, &sensorData, sizeof(sensorData), 0);
}

void IntermediaryNode::workerThread() {
    std::cout << "[IntermediaryNode] Hilo de trabajo iniciado" << std::endl;
    
    while (running_) {
        char buffer[BUFFER_SIZE];
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        // Usar select para timeout y poder verificar running_
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listen_sock_, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(listen_sock_ + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("select error");
            break;
        }
        
        if (!running_) break;
        
        if (activity > 0 && FD_ISSET(listen_sock_, &readfds)) {
            // Recibir datos de Arduino_node
            ssize_t n = recvfrom(listen_sock_, buffer, sizeof(buffer), 0,
                               reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            
            if (n < 0) {
                perror("recvfrom");
                continue;
            }

            // Verificar que tenemos un paquete completo (nuevo tamaño: 25 bytes)
            if (n == sizeof(SensorPacket)) {
                SensorPacket* packet = reinterpret_cast<SensorPacket*>(buffer);
                
                if (packet->msgId == 0x42) {  // SENSOR_DATA
                    processSensorPacket(*packet);
                } else {
                    std::cerr << "[IntermediaryNode] ID de mensaje desconocido: 0x" 
                              << std::hex << static_cast<int>(packet->msgId) << std::dec << std::endl;
                }
            } else {
                std::cerr << "[IntermediaryNode] Paquete de tamaño incorrecto: " << n 
                          << " bytes (esperaba " << sizeof(SensorPacket) << ")" << std::endl;
            }
        }
    }
    
    std::cout << "[IntermediaryNode] Hilo de trabajo terminado" << std::endl;
}

bool IntermediaryNode::start() {
    if (running_) {
        std::cout << "[IntermediaryNode] Ya está ejecutándose" << std::endl;
        return true;
    }

    std::cout << "[IntermediaryNode] Iniciando..." << std::endl;

    if (!createUdpSocket()) {
        std::cerr << "[IntermediaryNode] Error creando socket UDP" << std::endl;
        return false;
    }

    setupMasterConnection();

    running_ = true;
    worker_thread_ = std::thread(&IntermediaryNode::workerThread, this);

    std::cout << "[IntermediaryNode] Listo para recibir datos de Arduino_node..." << std::endl;
    return true;
}

void IntermediaryNode::stop() {
    if (!running_) return;
    
    std::cout << "[IntermediaryNode] Deteniendo..." << std::endl;
    running_ = false;
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
    
    if (listen_sock_ != -1) {
        close(listen_sock_);
        listen_sock_ = -1;
    }
    
    std::cout << "[IntermediaryNode] Detenido" << std::endl;
}