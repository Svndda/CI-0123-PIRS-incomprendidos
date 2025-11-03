#include "common/LogManager.h"
#include <iostream>
#include <unistd.h>

int main() {
    auto& logger = LogManager::instance();
    
    // Configurar logging remoto
    logger.configureRemote("127.0.0.1", 9999, "TestNode");
    
    std::cout << "Enviando logs de prueba al master en 127.0.0.1:9999..." << std::endl;
    
    // Enviar algunos logs de prueba
    logger.log(LogLevel::Info, "Test message 1: LogManager funcionando");
    logger.log(LogLevel::Warning, "Test message 2: Prueba de warning");
    logger.log(LogLevel::Critical, "Test message 3: Evento crítico de prueba");
    
    std::cout << "Logs enviados. Revisa el receptor UDP en puerto 9999." << std::endl;
    
    // Pequeña pausa para asegurar que los logs se envíen
    sleep(1);
    
    return 0;
}