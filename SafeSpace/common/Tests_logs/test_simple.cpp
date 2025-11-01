#include "common/LogManager.h"
#include <iostream>
#include <unistd.h>

int main() {
    auto& logger = LogManager::instance();
    
    std::cout << "=== PRUEBA DE LOGMANAGER UDP ===" << std::endl;
    
    // Configurar logging remoto
    logger.configureRemote("127.0.0.1", 8888, "TestNode");
    
    std::cout << "LogManager configurado para 127.0.0.1:8888" << std::endl;
    
    // Hacer un sleep pequeño para que el receptor esté listo
    sleep(1);
    
    std::cout << "Enviando log..." << std::endl;
    logger.log(LogLevel::Info, "MENSAJE DE PRUEBA DESDE LOGMANAGER");
    
    std::cout << "Log enviado. Esperando..." << std::endl;
    sleep(2);
    
    return 0;
}