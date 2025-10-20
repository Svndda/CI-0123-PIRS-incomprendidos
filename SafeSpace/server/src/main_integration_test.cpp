#include <iomanip>
#include "nodes/Auth/auth_udp_server.h"
#include "nodes/Proxy/ProxyNode.h"
#include "nodes/TestClient/test_udp_client.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <csignal>

static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

void runAuthServer() {
    AuthUDPServer server(8080);
    std::cout << " Iniciando AuthUDPServer en puerto 8080..." << std::endl;
    server.serveBlocking();
}

void runProxyNode() {
    ProxyNode proxy(9090, "127.0.0.1", 8080);
    std::cout << " Iniciando ProxyNode en puerto 9090..." << std::endl;
    proxy.start();
}

void runClientTests() {
    std::this_thread::sleep_for(std::chrono::seconds(2));

    TestUDPClient client("127.0.0.1", 9090);

    std::cout << "\n === PRUEBA 1: DISCOVER ===" << std::endl;
    client.sendDiscover(1, 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n === PRUEBA 2: AUTENTICACIÓN EXITOSA ===" << std::endl;
    client.sendAuthRequest(1001, "realAdmin", "M2sv8KxpLq");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n === PRUEBA 3: AUTENTICACIÓN FALLIDA ===" << std::endl;
    client.sendAuthRequest(1002, "admin", "wrongpassword");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "\n === PRUEBA 4: USUARIO NO EXISTENTE ===" << std::endl;
    client.sendAuthRequest(1003, "nonexistent", "password");
}

int main() {
    std::cout << " INICIANDO PRUEBA INTEGRADA CLIENTE-PROXY-AUTH" << std::endl;

    struct sigaction sa{};
    sa.sa_handler = sigHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // Iniciar servidor de autenticación
    std::thread auth_thread(runAuthServer);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Iniciar proxy
    std::thread proxy_thread(runProxyNode);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Ejecutar pruebas del cliente
    std::thread client_thread(runClientTests);
    client_thread.join();

    // Esperar a que el usuario presione Ctrl+C
    std::cout << "\n Presiona Ctrl+C para terminar..." << std::endl;
    while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << " Terminando prueba..." << std::endl;
    return 0;
}