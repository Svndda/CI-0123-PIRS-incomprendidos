#include "common/LogManager.h"
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Función simple para enviar UDP directamente
void testUDPSend() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        std::cerr << "Error creando socket" << std::endl;
        return;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    std::string msg = "HOLA DESDE UDP DIRECTO";
    int sent = sendto(sock, msg.c_str(), msg.length(), 0, 
                      (struct sockaddr*)&addr, sizeof(addr));
    
    std::cout << "UDP directo - Bytes enviados: " << sent << std::endl;
    if (sent < 0) {
        std::cerr << "Error: " << strerror(errno) << std::endl;
    }
    
    close(sock);
}

int main() {
    std::cout << "=== PRUEBA 1: UDP DIRECTO ===" << std::endl;
    testUDPSend();
    
    sleep(2);
    
    std::cout << "=== PRUEBA 2: LOGMANAGER ===" << std::endl;
    auto& logger = LogManager::instance();
    logger.configureRemote("127.0.0.1", 8888, "TestNode");
    logger.log(LogLevel::Info, "MENSAJE DESDE LOGMANAGER");
    
    sleep(2);
    return 0;
}