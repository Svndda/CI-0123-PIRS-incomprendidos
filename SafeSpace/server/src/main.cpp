#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include <csignal>
#include <iostream>

#include "Arduino/Arduino_Node.h"
#include "Auth/auth_udp_server.h"
#include "Bootstrap/Bootstrap.h"
#include "Bootstrap/BootstrapAdapters.h"
#include "Intermediary/IntermediaryNode.h"
#include "Storage/StorageNode.h"

#include <vector>
static volatile std::sig_atomic_t stopFlag = 0;
extern "C" void sigHandler(int) { stopFlag = 1; }

/**
 * @brief Validates and parses command-line arguments.
 */
void validateArgs(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage:\n"
              << "  " << argv[0] << " <component> [args...]\n"
              << "Components: server, proxy, storage, auth, events, inter, arduino, bootstrap\n"
              << "  For bootstrap you can optionally pass IP and PORT:\n"
              << "    " << argv[0] << " bootstrap <ip> <port>\n"
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

    if (type == "server") {
      if (argc != 10) {
        throw std::runtime_error("Master mode requires 9 arguments:"
        " server <local_ip> <local_port>"
        " <storageNode_ip> <storageNode_Port>"
        " <eventsNode_ip> <eventsNode_Port>"
        " <ProxyNode_ip> <ProxyNode_Port>");
      }

      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      std::string storageIp = argv[4];
      uint16_t storagePort = parsePort(argv[5]);
      std::string eventsIp = argv[6];
      uint16_t eventsPort = parsePort(argv[7]);
      std::string proxyIp = argv[8];
      uint16_t proxyPort = parsePort(argv[9]);

      SafeSpaceServer server(localIp, localPort, storageIp, storagePort, eventsIp, eventsPort, proxyIp, proxyPort);
      std::cout << "[Main] Running SafeSpaceServer on port " << localPort << std::endl;

      // Run server in blocking mode for master/server component
      server.serveBlocking();

      if (stopFlag) server.stop();
      std::cout << "[Main] Server stopped cleanly." << std::endl;

    } else if (type == "events") {
      if (argc != 5) {
        throw std::runtime_error("Events mode requires 4 arguments: events <local_ip> <local_port> <out.txt>");
      }

      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      std::string outPath = argv[4];
      CriticalEventsNode node(localIp, localPort, outPath);
      node.serveBlocking();

    } else if (type == "proxy") {
      if (argc != 8) {
        throw std::runtime_error("Proxy mode requires 7 arguments: proxy <local_ip> <local_port> <authNode_ip> <authNode_port> <masterNode_Ip> <masterNode_Port>");
      }

      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      std::string authNodeIp = argv[4];
      uint16_t authNodePort = parsePort(argv[5]);
      std::string masterNodeIp = argv[6];
      uint16_t masterNodePort = parsePort(argv[7]);

      std::cout << "Datos de AuthNode " << authNodeIp << ": " << authNodePort << std::endl;
      std::cout << "Datos de MasterNode " << masterNodeIp << ": " << masterNodePort << std::endl;

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
        throw std::runtime_error("Storage mode requires 6 arguments: storage <local_ip> <local_port> <masterNode_ip> <masterNode_port> <diskPath>");
      }

      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      const std::string masterIp = argv[4];
      const uint16_t masterPort = parsePort(argv[5]);
      const std::string nodeId = "storage1";
      const std::string diskPath = argv[6]; // Usar el archivo proporcionado

      // Crear instancia de StorageNode
      StorageNode storage(localPort, masterIp, masterPort, nodeId, diskPath);
      storage.start();
    } else if (type ==  "auth") {
      if (argc != 4) throw std::runtime_error("Auth mode requires 3 arguments: auth <local_ip> <local_port>");
      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      AuthUDPServer server(localIp, localPort);
      std::cout << " Iniciando AuthUDPServer en puerto: " << localPort << std::endl;
      server.serveBlocking();

    } else if (type == "inter") {
      if (argc != 5) {
        throw std::runtime_error("Intermediary mode requires 4 arguments: intermediary <local_ip> <local_port> <master_port>");
      }

      std::string localIp = argv[2];
      uint16_t localPort = parsePort(argv[3]);
      uint16_t interPort = parsePort(argv[4]);
      IntermediaryNode node(interPort, localIp, localPort);
      node.start();

      // Mantener proceso vivo
      while (!stopFlag) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
      }

    } else if (type == "arduino") {
      if (argc < 4) {
          std::cerr << "Uso: ./Arduino_Node <IP_NODO_MAESTRO> <PUERTO> [SERIAL_PATH|stdin|simulate] [format=json|binary|both]\n";
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
    } else if (type == "bootstrap") {
      // Bootstrap puede iniciarse con IP/PORT opcionales: `bootstrap <ip> <port>`
      std::string bindIp = "0.0.0.0";
      uint16_t bindPort = 8080;
      if (argc >= 4) {
        bindIp = argv[2];
        bindPort = parsePort(argv[3]);
      }

      Bootstrap server(bindIp, bindPort);

      // Registrar adaptadores para nodos comunes (IDs por convención)
      // ID 1: ProxyNode (escucha 9000), reenvía a Auth=7000 y Master=6000
      {
        auto p = makeProxyAdapter("0.0.0.0", 9000, "0.0.0.0", 7000, "127.0.0.1", 6000);
        server.registerNode(1, p.first, p.second);
      }

      // ID 2: StorageNode (puerto 9001), master en 127.0.0.1:6000
      {
        auto p = makeStorageAdapter(9001, "127.0.0.1", 6000, "storage1", "/tmp/storage1");
        server.registerNode(2, p.first, p.second);
      }

      // ID 3: IntermediaryNode (escucha 9002), master 127.0.0.1:6000
      {
        auto p = makeIntermediaryAdapter(9002, "127.0.0.1", 6000);
        server.registerNode(3, p.first, p.second);
      }

      // ID 4: AuthUDPServer (escucha 7000)
      {
        auto p = makeAuthAdapter("0.0.0.0", 7000);
        server.registerNode(4, p.first, p.second);
      }

      // ID 5: ArduinoNode (simulado) enviando a master 127.0.0.1:6000
      {
        auto p = makeArduinoAdapter("127.0.0.1", 9002, "simulate", "binary");
        server.registerNode(5, p.first, p.second);
      }

      // Run bootstrap in background so we can control nodes interactively
      std::thread serverThread([&server]() {
        server.serveBlocking();
      });

      std::cout << "Bootstrap interactive menu started. Commands: list | start <id> | stop <id> | quit" << std::endl;
      std::string line;
      while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;
        if (line == "list") {
          auto nodes = server.listNodes();
          std::cout << "Registered nodes:\n";
          for (auto &p : nodes) {
            std::cout << "  id=" << int(p.first) << " running=" << (p.second ? "yes" : "no") << std::endl;
          }
          continue;
        }
        if (line.rfind("start ", 0) == 0) {
          int id = std::stoi(line.substr(6));
          bool ok = server.startNode(static_cast<uint8_t>(id));
          std::cout << (ok ? "started" : "failed or unknown id") << std::endl;
          continue;
        }
        if (line.rfind("stop ", 0) == 0) {
          int id = std::stoi(line.substr(5));
          bool ok = server.stopNode(static_cast<uint8_t>(id));
          std::cout << (ok ? "stopped" : "failed or unknown id") << std::endl;
          continue;
        }
        if (line == "quit" || line == "exit") {
          std::cout << "Shutting down..." << std::endl;
          break;
        }
        std::cout << "Unknown command" << std::endl;
      }

      // Stop all nodes and the server
      server.stopAllNodes();
      server.stop();
      if (serverThread.joinable()) serverThread.join();
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