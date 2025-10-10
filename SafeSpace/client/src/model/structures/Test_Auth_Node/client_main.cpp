#include "client.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string server_ip = "127.0.0.1";
    int server_port = 8080;
    std::string username, password;

    // Parsear argumentos
    if (argc < 3) {
        std::cout << "Uso: " << argv[0] << " <usuario> <contraseña> [ip_servidor] [puerto]" << std::endl;
        std::cout << "Ejemplo: " << argv[0] << " admin admin123 192.168.1.100 8080" << std::endl;
        return 1;
    }

    username = argv[1];
    password = argv[2];

    if (argc >= 4) {
        server_ip = argv[3];
    }
    if (argc >= 5) {
        server_port = std::atoi(argv[4]);
    }

    std::cout << "🔌 Conectando a " << server_ip << ":" << server_port << std::endl;
    
    Client client(server_ip, server_port);
    if (!client.connectToServer()) {
        return 1;
    }

    AuthenticationResponse response = client.sendAuthentication(username, password);
    
    std::cout << "\n=== RESULTADO DE AUTENTICACIÓN ===" << std::endl;
    std::cout << "Usuario: " << username << std::endl;
    std::cout << response << std::endl;
    
    if (response.getDataType() == AuthenticationResponse::AUTH_SUCCESS) {
        std::cout << " Autenticación EXITOSA" << std::endl;
    } else {
        std::cout << " Autenticación FALLIDA" << std::endl;
    }
    std::cout << "===================================" << std::endl;

    client.disconnect();
    return 0;
}