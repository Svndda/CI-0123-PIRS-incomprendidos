#include "SafeSpaceServer.h"
#include "Proxy/ProxyNode.h"
#include "nodes/Storage/StorageNode.h"
#include <csignal>
#include <iostream>
#include <cstring>

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

std::vector<uint8_t> sensorDataToBytes(const SensorData& data) {
    std::vector<uint8_t> bytes;
    bytes.reserve(24);
    
    std::cout << "[Test] Converting SensorData to bytes:" << std::endl;
    std::cout << "[Test] Distance: " << data.distance << std::endl;
    
    float values[6] = {data.distance, data.temperature, data.pressure, 
                      data.altitude, data.sealevelPressure, data.realAltitude};
    
    for (int i = 0; i < 6; i++) {
        uint32_t host_value;
        std::memcpy(&host_value, &values[i], 4);
        uint32_t net_value = htonl(host_value);
        bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&net_value), 
                    reinterpret_cast<uint8_t*>(&net_value) + 4);
    }
    
    std::cout << "[Test] Generated " << bytes.size() << " bytes" << std::endl;
    return bytes;
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
      /*
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
      */
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
        
        std::cout << "\n=== Testing StorageNode with New SensorData ===\n" << std::endl;

        // Test 1: Crear y almacenar datos de prueba con la nueva estructura
        std::cout << "Test 1: Creating test SensorData..." << std::endl;
        
        // Crear múltiples datos de sensores de prueba
        SensorData testData1(150.5f, 25.5f, 1013.25f, 100.0f, 1013.25f, 99.8f);
        SensorData testData2(200.3f, 26.1f, 1012.80f, 105.2f, 1013.25f, 104.9f);
        SensorData testData3(75.8f, 24.8f, 1013.50f, 98.7f, 1013.25f, 98.5f);
            
        // Almacenar datos de prueba
        std::cout << "Storing test data 1..." << std::endl;
        std::vector<uint8_t> message1;
        message1.push_back(static_cast<uint8_t>(MessageType::STORE_SENSOR_DATA));
        auto sensorBytes1 = sensorDataToBytes(testData1);
        message1.insert(message1.end(), sensorBytes1.begin(), sensorBytes1.end());
        
        std::string response1;
        storage.testReceive(message1.data(), message1.size(), response1);

        std::cout << "Storing test data 2..." << std::endl;
        std::vector<uint8_t> message2;
        message2.push_back(static_cast<uint8_t>(MessageType::STORE_SENSOR_DATA));
        auto sensorBytes2 = sensorDataToBytes(testData2);
        message2.insert(message2.end(), sensorBytes2.begin(), sensorBytes2.end());

        std::string response2;
        storage.testReceive(message2.data(), message2.size(), response2);

        std::cout << "Storing test data 3..." << std::endl;
        std::vector<uint8_t> message3;
        message3.push_back(static_cast<uint8_t>(MessageType::STORE_SENSOR_DATA));
        auto sensorBytes3 = sensorDataToBytes(testData3);
        message3.insert(message3.end(), sensorBytes3.begin(), sensorBytes3.end());
        
        std::string response3;
        storage.testReceive(message3.data(), message3.size(), response3);

      // Verificar respuestas de almacenamiento
        auto checkResponse = [](const std::string& response, const std::string& testName) {
          if (response.size() > 1) {
            uint8_t status = response[1];
            if (status == 0) {
              std::cout << testName << " - SUCCESS" << std::endl;
              return true;
            } else {
              std::cout << testName << " - FAILED with status: " << static_cast<int>(status) << std::endl;
              return false;
            }
          }
          std::cout << testName << " - INVALID RESPONSE" << std::endl;
          return false;
        };

        checkResponse(response1, "Test Data 1");
        checkResponse(response2, "Test Data 2");
        checkResponse(response3, "Test Data 3");

        // Test 2: Consultar por fecha
        std::cout << "\nTest 2: Querying by date range..." << std::endl;
        uint64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        uint64_t start = now - 3600; // última hora
        uint64_t end = now + 3600;   // próxima hora

        std::vector<uint8_t> queryDateMsg;
        queryDateMsg.push_back(static_cast<uint8_t>(MessageType::QUERY_BY_DATE));
        
        // Agregar timestamps en network byte order
        uint64_t startNet = htobe64(start);
        uint64_t endNet = htobe64(end);
        const uint8_t* startBytes = reinterpret_cast<const uint8_t*>(&startNet);
        const uint8_t* endBytes = reinterpret_cast<const uint8_t*>(&endNet);
       
        queryDateMsg.insert(queryDateMsg.end(), startBytes, startBytes + 8);
        queryDateMsg.insert(queryDateMsg.end(), endBytes, endBytes + 8);

        std::string queryDateResponse;
        storage.testReceive(queryDateMsg.data(), queryDateMsg.size(), queryDateResponse);

            // Procesar respuesta de consulta por fecha
        if (queryDateResponse.size() > 2) {
          uint8_t status = queryDateResponse[1];
          if (status == 0) {
            std::cout << "Query by date - SUCCESS" << std::endl;
            std::cout << "Response size: " << queryDateResponse.size() << " bytes" << std::endl;
            
            size_t dataSize = queryDateResponse.size() - 2;
            if (dataSize > 0) {
              std::cout << "Received " << dataSize << " bytes of sensor data (" 
                        << (dataSize / 24) << " records)" << std::endl;
            } else {
              std::cout << "No data found in the specified date range" << std::endl;
            }
          } else {
            std::cout << "Query by date - FAILED with status: " << static_cast<int>(status) << std::endl;
          }
       }

            // Test 3: Consultar por ID de sensor (usando ID 0 por defecto)
        std::cout << "\nTest 3: Querying by sensor ID..." << std::endl;
        std::vector<uint8_t> querySensorMsg;
        querySensorMsg.push_back(static_cast<uint8_t>(MessageType::QUERY_BY_SENSOR));
        querySensorMsg.push_back(0); // sensorId (0 por defecto en nuestras pruebas)
        
        // Agregar timestamps
        querySensorMsg.insert(querySensorMsg.end(), startBytes, startBytes + 8);
        querySensorMsg.insert(querySensorMsg.end(), endBytes, endBytes + 8);

        std::string querySensorResponse;
        storage.testReceive(querySensorMsg.data(), querySensorMsg.size(), querySensorResponse);

             // Procesar respuesta de consulta por sensor
        if (querySensorResponse.size() > 2) {
          uint8_t status = querySensorResponse[1];
          if (status == 0) {
            std::cout << "Query by sensor - SUCCESS" << std::endl;
            std::cout << "Response size: " << querySensorResponse.size() << " bytes" << std::endl;
            
            size_t dataSize = querySensorResponse.size() - 2;
            if (dataSize > 0) {
              std::cout << "Received " << dataSize << " bytes of sensor data (" 
                        << (dataSize / 24) << " records)" << std::endl;
            } else {
              std::cout << "No data found for the specified sensor ID" << std::endl;
            }
          } else {
            std::cout << "Query by sensor - FAILED with status: " << static_cast<int>(status) << std::endl;
          }
        }

    // Test 4: Almacenar bitácora - DEBE ser el primer byte del mensaje
      std::cout << "\nTest 4: Storing bitacora entry..." << std::endl;
      std::vector<uint8_t> bitacoraMsg;
      bitacoraMsg.push_back(static_cast<uint8_t>(MessageType::STORE_BITACORA));  // Este es el tipo
      std::string bitacoraText = "Test log entry from StorageNode test";
      bitacoraMsg.insert(bitacoraMsg.end(), bitacoraText.begin(), bitacoraText.end());

      std::cout << "[Test] Bitacora message size: " << bitacoraMsg.size() << " bytes" << std::endl;
      std::cout << "[Test] First byte (type): " << static_cast<int>(bitacoraMsg[0]) << std::endl;

      std::string bitacoraResponse;
      storage.testReceive(bitacoraMsg.data(), bitacoraMsg.size(), bitacoraResponse);

      if (bitacoraResponse.size() > 1) {
          uint8_t status = bitacoraResponse[1];
          if (status == 0) {
              std::cout << "Bitacora storage - SUCCESS" << std::endl;
          } else {
              std::cout << "Bitacora storage - FAILED with status: " << static_cast<int>(status) << std::endl;
          }
      }

      // Mostrar estadísticas finales
      auto stats = storage.getStats();
      std::cout << "\n=== Storage Node Final Statistics ===" << std::endl;
      std::cout << "Total sensor records: " << stats.totalSensorRecords << std::endl;
      std::cout << "Total queries: " << stats.totalQueries << std::endl;
      std::cout << "Total errors: " << stats.errorsCount << std::endl;

      std::cout << "\n=== StorageNode Tests Completed Successfully ===\n" << std::endl;

       // Esperar un poco para ver los resultados
      std::this_thread::sleep_for(std::chrono::seconds(1));

    
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
