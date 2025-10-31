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
    // Convertir de network byte order a host byte order
    int16_t temp_raw = ntohs(static_cast<uint16_t>(packet.temp_x100));
    int16_t hum_raw = ntohs(static_cast<uint16_t>(packet.hum_x100));
    int16_t distance_raw = ntohs(static_cast<uint16_t>(packet.distance_x100));
    int32_t pressure_raw = ntohl(static_cast<uint32_t>(packet.pressure_pa));
    int16_t altitude_raw = ntohs(static_cast<uint16_t>(packet.altitude_x100));
    
    // Convertir a valores reales
    double temperature = static_cast<double>(temp_raw) / 100.0;
    double humidity = static_cast<double>(hum_raw) / 100.0;
    double distance = static_cast<double>(distance_raw) / 100.0;
    double pressure = static_cast<double>(pressure_raw);
    double altitude = static_cast<double>(altitude_raw) / 100.0;

    std::cout << "[IntermediaryNode] Datos recibidos:" << std::endl;
    std::cout << "  - Temperatura: " << temperature << " °C" << std::endl;
    std::cout << "  - Humedad: " << humidity << " %" << std::endl;
    std::cout << "  - Distancia: " << distance << " m" << std::endl;
    std::cout << "  - Presión: " << pressure << " Pa" << std::endl;
    std::cout << "  - Altitud: " << altitude << " m" << std::endl;

    // Crear objeto SensorData (usando los campos disponibles)
    // Nota: El paquete actual no incluye sealevelPressure ni realAltitude
    // Por ahora usamos los mismos valores o valores por defecto
    SensorData sensorData(
        distance,      // distance (convertido a float)
        temperature,   // temperature  
        pressure,      // pressure
        altitude,      // altitude
        pressure,      // sealevelPressure (usamos pressure como placeholder)
        altitude       // realAltitude (usamos altitude como placeholder)
    );

    std::cout << "[IntermediaryNode] Preparado para enviar SensorData al Master_node:" << std::endl;
    std::cout << "[IntermediaryNode] - Distancia: " << sensorData.distance << " m" << std::endl;
    std::cout << "[IntermediaryNode] - Temperatura: " << sensorData.temperature << " °C" << std::endl;
    std::cout << "[IntermediaryNode] - Presión: " << sensorData.pressure << " Pa" << std::endl;
    std::cout << "[IntermediaryNode] - Altitud: " << sensorData.altitude << " m" << std::endl;
    
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

            // Verificar que tenemos un paquete completo (nuevo tamaño: 13 bytes)
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
                std::cerr << "[IntermediaryNode] ¿Está el ArduinoNode configurado en modo binary?" << std::endl;
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
    std::cout << "[IntermediaryNode] Esperando paquetes binarios de " << sizeof(SensorPacket) << " bytes" << std::endl;
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