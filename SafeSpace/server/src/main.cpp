#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include "nodes/Storage/StorageNode.h"
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
              << "  " << argv[0] << " storage <disk_file>\n"
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

  try {
    if (type == "server") {
      if (argc != 3) {
        throw std::runtime_error("Server mode requires exactly 2 arguments.");
      }

      uint16_t localPort = parsePort(argv[2]);
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

      uint16_t localPort = parsePort(argv[2]);
      std::string serverIp = argv[3];
      uint16_t serverPort = parsePort(argv[4]);

      ProxyNode proxy(localPort, serverIp, serverPort);
      std::cout << "[Main] Running ProxyNode on port " << localPort
                << " → forwarding to " << serverIp << ":" << serverPort << std::endl;

      proxy.start();

      if (stopFlag) proxy.stop();
      std::cout << "[Main] ProxyNode stopped cleanly." << std::endl;

    } else if (type == "storage") {
            if (argc != 3)
                throw std::runtime_error("Storage mode requires the disk file path.");

            // Configuración del Storage Node
            const uint16_t storagePort = 6000;
            const std::string masterIp = "127.0.0.1";
            const uint16_t masterPort = 5000;
            const std::string nodeId = "storage1";
            const std::string diskPath = argv[2]; // Usar el archivo proporcionado

            // Crear instancia de StorageNode
            StorageNode storage(storagePort, masterIp, masterPort, nodeId, diskPath);
            
            std::cout << "\n=== Testing StorageNode Connectivity ===\n" << std::endl;

            // Test 1: Crear y almacenar datos de prueba
            std::cout << "Test 1: Creating test data..." << std::endl;
            SensorData testData{
                .timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()),
                .distance = 150,
                .movement = 1,
                .temperature = 2500, // 25.00°C
                .uv = 500,
                .microphone = 300,
                .leds = 0x03,
                .buzzer = 1,
                .light = 800,
                .sensorId = 1
            };
            
            // Convertir a bytes para simular mensaje
            std::vector<uint8_t> message;
            message.push_back(static_cast<uint8_t>(MessageType::STORE_SENSOR_DATA));
            auto sensorBytes = testData.toBytes();
            message.insert(message.end(), sensorBytes.begin(), sensorBytes.end());

            // Simular recepción de mensaje
            sockaddr_in testPeer{};
            std::string response;
            storage.testReceive(message.data(), message.size(), response);

            // Test 2: Consultar datos
            std::cout << "\nTest 2: Querying stored data..." << std::endl;
            uint64_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            uint64_t start = now - 3600; // última hora

            std::vector<uint8_t> queryMsg;
            queryMsg.push_back(static_cast<uint8_t>(MessageType::QUERY_BY_SENSOR));
            queryMsg.push_back(1); // sensorId
            
            // Agregar timestamps en network byte order
            uint64_t startNet = htobe64(start);
            uint64_t endNet = htobe64(now);
            const uint8_t* startBytes = reinterpret_cast<const uint8_t*>(&startNet);
            const uint8_t* endBytes = reinterpret_cast<const uint8_t*>(&endNet);
            
            queryMsg.insert(queryMsg.end(), startBytes, startBytes + 8);
            queryMsg.insert(queryMsg.end(), endBytes, endBytes + 8);

            std::string queryResponse;
            storage.testReceive(queryMsg.data(), queryMsg.size(), queryResponse);

            // Mostrar estadísticas
            auto stats = storage.getStats();
            std::cout << "\nStorage Node Statistics:" << std::endl;
            std::cout << "Total records: " << stats.totalSensorRecords << std::endl;
            std::cout << "Total queries: " << stats.totalQueries << std::endl;
            std::cout << "Errors: " << stats.errorsCount << std::endl;

            std::cout << "\n=== StorageNode Tests Completed ===\n" << std::endl;

    
    } else {
        throw std::runtime_error("Invalid component type: " + type +
                                    " (must be 'server', 'proxy', or 'storage')");
    }        

  } catch (const std::exception& ex) {
    std::cerr << "[Fatal] " << ex.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
