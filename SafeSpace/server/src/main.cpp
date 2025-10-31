#include <iostream>
#include <csignal>
#include <atomic>
#include "nodes/Intermediary/IntermediaryNode.h"

std::atomic<bool> running{true};

void signalHandler(int signal) {
    std::cout << "\n[Main] Recibida señal de interrupción. Cerrando..." << std::endl;
    running = false;
}

void printUsage(const char* program_name) {
    std::cout << "Uso: " << program_name << " <PUERTO_ESCUCHA> <MASTER_IP> <MASTER_PUERTO>" << std::endl;
    std::cout << "Ejemplo: " << program_name << " 9999 127.0.0.1 8888" << std::endl;
    std::cout << "O usar valores por defecto: " << program_name << std::endl;
}

int main(int argc, char* argv[]) {
    // Configurar manejador de señales
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Valores por defecto
    int listen_port = 9999;
    std::string master_ip = "127.0.0.1";
    int master_port = 8888;
    
    // Parsear argumentos de línea de comandos
    if (argc == 4) {
        listen_port = std::stoi(argv[1]);
        master_ip = argv[2];
        master_port = std::stoi(argv[3]);
    } else if (argc != 1) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::cout << "==========================================" << std::endl;
    std::cout << "    INTERMEDIARY NODE - SAFESPACE" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Puerto escucha: " << listen_port << std::endl;
    std::cout << "Master node: " << master_ip << ":" << master_port << std::endl;
    std::cout << "Esperando paquetes binarios de ArduinoNode" << std::endl;
    std::cout << "Tamaño esperado: 13 bytes" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Campos esperados:" << std::endl;
    std::cout << "- Temperatura (°C)" << std::endl;
    std::cout << "- Humedad (%)" << std::endl;
    std::cout << "- Distancia (m)" << std::endl;
    std::cout << "- Presión (Pa)" << std::endl;
    std::cout << "- Altitud (m)" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Para probar, ejecuta ArduinoNode en modo binary:" << std::endl;
    std::cout << "./ArduinoNode 127.0.0.1 " << listen_port << " simulate binary" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Presiona Ctrl+C para terminar" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    // Crear e iniciar el nodo intermediario
    IntermediaryNode node(listen_port, master_ip, master_port);
    
    if (!node.start()) {
        std::cerr << "Error: No se pudo iniciar el IntermediaryNode" << std::endl;
        return 1;
    }
    
    // Bucle principal de espera
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    // Detener el nodo
    node.stop();
    
    std::cout << "Aplicación terminada correctamente." << std::endl;
    return 0;
}