#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>

#include "Auth/auth_udp_server.h"

static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

/**
 * @brief Validates and parses command-line arguments.
 */
void validateArgs(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage:\n"
              << "  " << argv[0] << " server <local_port>\n"
              << "  " << argv[0] << " proxy <local_port> <server_ip> <server_port>\n"
              << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

/**
 * @brief Converts a string to a valid port number (uint16_t).
 */
uint16_t parsePort(const std::string& str) {
  int port = std::stoi(str);
  if (port <= 0 || port > 65535) {
    throw std::invalid_argument("Invalid port number: " + str);
  }
  return static_cast<uint16_t>(port);
}

int main(const int argc, char* argv[]) {
  validateArgs(argc, argv);

  struct sigaction sa{};
  sa.sa_handler = sigHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);

  try {
    std::string type = argv[1];
    uint16_t localPort = parsePort(argv[3]);
    std::string localIp = argv[2];

    if (type == "server") {
      if (argc != 3) {
        throw std::runtime_error("Server mode requires exactly 2 arguments.");
      }

      SafeSpaceServer server(localIp, localPort);
      std::cout << "[Main] Running SafeSpaceServer on port " << localPort << std::endl;

      // // Ejemplo: registrar un destino de descubrimiento local (opcional)
      // server.addDiscoverTarget("127.0.0.1", 6000);

      server.serveBlocking();

      if (stopFlag) server.stop();
      std::cout << "[Main] Server stopped cleanly." << std::endl;

    } else if (type == "proxy") {
      if (argc != 8) {
        throw std::runtime_error("Proxy mode requires 7 arguments:"
        " proxy <local_ip> <local_port>"
        " <authNode_ip> <authNode_port>"
        " <masterNode_Ip> <masterNode_Port>"
        );
      }

      std::string authNodeIp = argv[4];
      uint16_t authNodePort = parsePort(argv[5]);
      std::string masterNodeIp = argv[6];
      uint16_t masterNodePort = parsePort(argv[7]);


      std::cout << "Datos de AuthNode" << authNodeIp << ": " << authNodePort << std::endl;
      std::cout << "Dtos de MasterNode" << masterNodeIp << ": " << masterNodePort << std::endl;


      ProxyNode proxy(
        localIp, localPort,
        authNodeIp, authNodePort,
        masterNodeIp, masterNodePort
      );

      std::cout << "[Main] Running ProxyNode on ip" << localIp
        <<  " and port " << localPort
        << " → forwarding to AuthNode " << authNodeIp << ":" << authNodePort
        << "and  → forwarding to MasterNode " << masterNodeIp << masterNodePort << std::endl;

      proxy.start();

      if (stopFlag) proxy.stop();
      std::cout << "[Main] ProxyNode stopped cleanly." << std::endl;

    } else if (type ==  "auth") {
      AuthUDPServer server(localIp, localPort);
      std::cout << " Iniciando AuthUDPServer en puerto: " << localPort << std::endl;
      server.serveBlocking();
    } else {
      throw std::runtime_error("Invalid component type: " + type +
                               " (must be 'server' or 'proxy')");
    }

  } catch (const std::exception& ex) {
    std::cerr << "[Fatal] " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
