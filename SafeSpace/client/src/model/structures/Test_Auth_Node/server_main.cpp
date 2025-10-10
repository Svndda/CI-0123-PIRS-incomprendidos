#include "auth_server.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

AuthServer* server = nullptr;

void signalHandler(int signal) {
    std::cout << "\n🛑 Recibida señal de interrupción. Cerrando servidor..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc >= 2) {
        port = std::atoi(argv[1]);
    }
    
    std::cout << "Iniciando servidor de autenticación..." << std::endl;
    
    // Manejar señales de interrupción
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    AuthServer authServer(port);
    server = &authServer;
    
    if (!authServer.start()) {
        std::cerr << "No se pudo iniciar el servidor" << std::endl;
        return 1;
    }
    
    authServer.run();
    return 0;
}