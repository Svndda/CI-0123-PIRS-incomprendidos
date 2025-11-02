#include "common/LogManager.h"
#include <iostream>
#include <unistd.h>

int main() {
    auto& logger = LogManager::instance();
    
    // Configurar logging remoto en puerto 8888
    logger.configureRemote("127.0.0.1", 8888, "TestNode");
    
    std::cout << "Enviando logs de prueba al master en 127.0.0.1:8888..." << std::endl;
    
    // Enviar algunos logs de prueba
    logger.log(LogLevel::Info, "Test message 1: LogManager funcionando");
    sleep(1);
    logger.log(LogLevel::Warning, "Test message 2: Prueba de warning");
    sleep(1);
    logger.log(LogLevel::Critical, "Test message 3: Evento crítico de prueba");
    
    std::cout << "Logs enviados." << std::endl;
    
    // Pausa para asegurar que los logs se envíen
    sleep(2);
    
    return 0;
}