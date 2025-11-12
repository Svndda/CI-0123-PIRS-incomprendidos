#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>

#include "Arduino/Arduino_Node.h"
#include "Auth/auth_udp_server.h"
#include "Intermediary/IntermediaryNode.h"
#include "Storage/StorageNode.h"

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
    std::string localIp = argv[2];
    uint16_t localPort = parsePort(argv[3]);

    if (type == "server") {
      if (argc != 10) {
        throw std::runtime_error("Master mode requires 8 arguments:"
        " server <local_ip> <local_port>"
        " <storageNode_ip> <storageNode_Port>"
        " <eventsNode_ip> <eventsNode_Port>"
        " <ProxyNode_ip> <ProxyNode_Port>"
        );
      }

      std::string storageIp = argv[4];
      uint16_t storagePort = parsePort(argv[5]);
      std::string eventsIp = argv[6];
      uint16_t eventsPort = parsePort(argv[7]);
      std::string proxyIp = argv[8];
      uint16_t proxyPort = parsePort(argv[9]);

      SafeSpaceServer server(localIp, localPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort);
      std::cout << "[Main] Running SafeSpaceServer on port " << localPort << std::endl;

      // // Ejemplo: registrar un destino de descubrimiento local (opcional)
      // server.addDiscoverTarget("127.0.0.1", 6000);

      server.serveBlocking();

      if (stopFlag) server.stop();
      std::cout << "[Main] Server stopped cleanly." << std::endl;

    } else if (type == "events") {
      if (argc != 5) {
        throw std::runtime_error("Events mode requires 3 arguments:"
        " events <local_ip> <local_port>" "out.txt"
        );
      }

      std::string outPath = argv[4];
      CriticalEventsNode node(localIp, localPort, outPath);
      node.serveBlocking();

    } else if (type == "proxy") {
      if (argc != 8) {
        throw std::runtime_error("Proxy mode requires 6 arguments:"
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

    } else if (type == "storage") {

      if (argc != 7) {
        throw std::runtime_error("Proxy mode requires 5 arguments:"
        " storage <local_ip> <local_port>"
        " <masterNode_ip> <masterNode_port>"
        " <diskPath>"
        );
      }

      const std::string masterIp = argv[4];
      const uint16_t masterPort = parsePort(argv[5]);
      const std::string nodeId = "storage1";
      const std::string diskPath = argv[6]; // Usar el archivo proporcionado
          
      // Crear instancia de StorageNode
      StorageNode storage(localPort, masterIp, masterPort, nodeId, diskPath);
      storage.start();
    } else if (type ==  "auth") {
      AuthUDPServer server(localIp, localPort);
      std::cout << " Iniciando AuthUDPServer en puerto: " << localPort << std::endl;
      server.serveBlocking();

    } else if (type == "inter") {
      if (argc != 5) {
        throw std::runtime_error("Proxy mode requires 3 arguments:"
        " intermediary <masterNode_ip> <masterNode_port> <local_port> "
        );
      }

      uint16_t interPort = parsePort(argv[4]);
      IntermediaryNode node(interPort, localIp, localPort);
      node.start();

      // Mantener proceso vivo
      while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }

    } else if (type == "arduino") {
      if (argc < 4) {
          std::cerr << "Uso: ./Arduino_Node <IP_NODO_MAESTRO> <PUERTO> [SERIAL_PATH|stdin|simulate] format=json|binary|both]\n";
          return 1;
      }

      std::string masterIP = argv[2];
      int masterPort = parsePort(argv[3]);
      std::string serialPath = "";
      std::string mode;
      if (argc >= 5) serialPath = argv[4];
      if (argc >= 6) mode = argv[5];

     ArduinoNode node(masterIP, masterPort, serialPath, mode);
     node.run();
    } else {
      throw std::runtime_error("Invalid component type: " + type +
                               " (must be 'server', 'storage' , 'proxy', 'auth', 'events', 'inter' , 'arduino')");
    }
  } catch (const std::exception& ex) {
    std::cerr << "[Fatal] " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}