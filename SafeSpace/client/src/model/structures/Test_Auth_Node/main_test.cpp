// main_test.cpp
#include "client.h"
#include "auth_server.h"
#include <thread>
#include <chrono>
#include <vector>

void runServer() {
    AuthServer server(8080);
    if (server.start()) {
        server.run();
    }
}

void testClient(const std::string& username, const std::string& password) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    Client client("127.0.0.1", 8080);
    if (client.connectToServer()) {
        AuthenticationResponse response = client.sendAuthentication(username, password);
        
        std::cout << "\n=== RESULTADO DE AUTENTICACIÓN ===" << std::endl;
        std::cout << "Usuario: " << username << std::endl;
        std::cout << response << std::endl;
        
        if (response.getDataType() == AuthenticationResponse::AUTH_SUCCESS) {
            std::cout << "✅ Autenticación EXITOSA" << std::endl;
        } else {
            std::cout << "❌ Autenticación FALLIDA" << std::endl;
        }
        std::cout << "===================================\n" << std::endl;
    }
    
    client.disconnect();
}

int main() {
    std::cout << "INICIANDO PRUEBA DE AUTENTICACIÓN CLIENTE-SERVIDOR\n" << std::endl;
    
    // Iniciar servidor en hilo separado
    std::thread server_thread(runServer);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Pruebas de autenticación
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {"admin", "admin123"},        // ✅ Debe funcionar
        {"auditor", "auditor123"},    // ✅ Debe funcionar  
        {"admin", "wrongpassword"},   // ❌ Debe fallar
        {"nonexistent", "password"},  // ❌ Debe fallar
        {"datamanager", "data123"},   // ✅ Debe funcionar
        {"guest", "guest123"}         // ✅ Debe funcionar
    };
    
    std::vector<std::thread> client_threads;
    
    for (const auto& test_case : test_cases) {
        client_threads.emplace_back(testClient, test_case.first, test_case.second);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    // Esperar a que todos los clientes terminen
    for (auto& thread : client_threads) {
        thread.join();
    }
    
    // Dar tiempo para ver los logs del servidor
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "Prueba completada. Cerrando servidor..." << std::endl;
    
    return 0;
}