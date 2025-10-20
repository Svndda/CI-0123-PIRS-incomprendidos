#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>

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

  std::string type = argv[1];
  uint16_t localPort = parsePort(argv[2]);

  try {
    if (type == "server") {
      if (argc != 3) {
        throw std::runtime_error("Server mode requires exactly 2 arguments.");
      }

      SafeSpaceServer server(localPort);
      std::cout << "[Main] Running SafeSpaceServer on port " << localPort << std::endl;

      // Ejemplo: registrar un destino de descubrimiento local (opcional)
      server.addDiscoverTarget("127.0.0.1", 6000);

      server.serveBlocking();

      if (stopFlag) server.stop();
      std::cout << "[Main] Server stopped cleanly." << std::endl;

    } else if (type == "proxy") {
      if (argc != 5) {
        throw std::runtime_error("Proxy mode requires 4 arguments: proxy <local_port> <server_ip> <server_port>");
      }

      std::string serverIp = argv[3];
      uint16_t serverPort = parsePort(argv[4]);

      ProxyNode proxy(localPort, serverIp, serverPort);
      std::cout << "[Main] Running ProxyNode on port " << localPort
                << " → forwarding to " << serverIp << ":" << serverPort << std::endl;

      proxy.start();

      if (stopFlag) proxy.stop();
      std::cout << "[Main] ProxyNode stopped cleanly." << std::endl;

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
