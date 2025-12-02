#include "StorageNode.h"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <regex>
#include <sstream>

const size_t BUFFER_SIZE = 65535;

// --- función auxiliar para decodificar texto hex a bytes ---
static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t) strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<uint8_t> StorageNode::sensorDataToBytes(const SensorData& data) const {
    std::vector<uint8_t> bytes;
    bytes.reserve(24); // 6 floats * 4 bytes = 24 bytes
    
    // Convertir cada float a bytes (network byte order)
    uint32_t dist_net = htonl(*reinterpret_cast<const uint32_t*>(&data.distance));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&dist_net), 
                reinterpret_cast<uint8_t*>(&dist_net) + 4);
    
    uint32_t temp_net = htonl(*reinterpret_cast<const uint32_t*>(&data.temperature));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&temp_net), 
                reinterpret_cast<uint8_t*>(&temp_net) + 4);
    
    uint32_t press_net = htonl(*reinterpret_cast<const uint32_t*>(&data.pressure));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&press_net), 
                reinterpret_cast<uint8_t*>(&press_net) + 4);
    
    uint32_t alt_net = htonl(*reinterpret_cast<const uint32_t*>(&data.altitude));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&alt_net), 
                reinterpret_cast<uint8_t*>(&alt_net) + 4);
    
    uint32_t seal_net = htonl(*reinterpret_cast<const uint32_t*>(&data.sealevelPressure));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&seal_net), 
                reinterpret_cast<uint8_t*>(&seal_net) + 4);
    
    uint32_t real_net = htonl(*reinterpret_cast<const uint32_t*>(&data.realAltitude));
    bytes.insert(bytes.end(), reinterpret_cast<uint8_t*>(&real_net), 
                reinterpret_cast<uint8_t*>(&real_net) + 4);
    
    return bytes;
}

SensorData StorageNode::bytesToSensorData(const uint8_t* data, size_t len) const {
    std::cout << "[StorageNode] bytesToSensorData - len: " << len << std::endl;
    
    if (len != sizeof(SensorData)) {
        std::cerr << "[StorageNode] Invalid sensor data length: " << len 
                  << " (expected 24 bytes)" << std::endl;
        throw std::runtime_error("Invalid sensor data length");
    }
    
    try {
        // Usar memcpy para evitar problemas de alineación
        float values[6];
        
        for (int i = 0; i < 6; i++) {
            float f;
            std::memcpy(&f, data + (i * 4), 4);
            values[i] = f; 
        }
        
        std::cout << "[StorageNode] Parsed values - "
                  << "Dist:" << values[0] << " Temp:" << values[1]
                  << " Pres:" << values[2] << " Alt:" << values[3]
                  << " SealPres:" << values[4] << " RealAlt:" << values[5] << std::endl;
        
        return SensorData(values[0], values[1], values[2], values[3], values[4], values[5]);
    } catch (const std::exception& e) {
        std::cerr << "[StorageNode] Error in bytesToSensorData: " << e.what() << std::endl;
        throw;
    }
}

std::string StorageNode::sensorDataToString(const SensorData& data) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << data.distance << ","
        << data.temperature << ","
        << data.pressure << ","
        << data.altitude << ","
        << data.sealevelPressure << ","
        << data.realAltitude;
    return oss.str();
}

SensorData StorageNode::stringToSensorData(const std::string& str) const {
    std::cout << "[StorageNode] stringToSensorData parsing: " << str << std::endl;
    
    // Si hay múltiples líneas, tomar solo la primera
    std::string firstLine = str;
    size_t newlinePos = str.find('\n');
    if (newlinePos != std::string::npos) {
        firstLine = str.substr(0, newlinePos);
        std::cout << "[StorageNode] Multiple lines found, using first line: " << firstLine << std::endl;
    }
    
    std::istringstream iss(firstLine);
    std::string token;
    std::vector<float> values;
    
    while (std::getline(iss, token, ',')) {
        try {
            float value = std::stof(token);
            values.push_back(value);
            std::cout << "[StorageNode] Parsed value: " << value << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[StorageNode] Error parsing token: " << token << " - " << e.what() << std::endl;
            throw;
        }
    }
    
    if (values.size() != 6) {
        std::cerr << "[StorageNode] Invalid sensor data string format. Expected 6 values, got " 
                  << values.size() << std::endl;
        throw std::runtime_error("Invalid sensor data string format");
    }
    
    return SensorData(values[0], values[1], values[2], values[3], values[4], values[5]);
}

std::vector<uint8_t> Response::toBytes() const {
    std::vector<uint8_t> bytes;
    bytes.reserve(2 + data.size());
    bytes.push_back(msgId);
    bytes.push_back(status);
    bytes.insert(bytes.end(), data.begin(), data.end());
    return bytes;
}

StorageNode::StorageNode(uint16_t storagePort, const std::string& masterServerIp,
                         uint16_t masterServerPort, const std::string& nodeId,
                         const std::string& diskPath, size_t bufsize)
    : UDPServer("0.0.0.0", storagePort, bufsize),
      masterClient(nullptr),
      masterServerIp(masterServerIp),
      masterServerPort(masterServerPort),
      nodeId(nodeId),
      diskPath(diskPath),
      listening(false),
      totalSensorRecords(0),
      totalQueries(0),
      errorsCount(0)
{
    std::cout << "[StorageNode] Initializing node: " << nodeId << std::endl;
    std::cout << "[StorageNode] Storage port: " << storagePort << std::endl;
    std::cout << "[StorageNode] Master server: " << masterServerIp 
              << ":" << masterServerPort << std::endl;
    std::cout << "[StorageNode] Disk path: " << diskPath << std::endl;
    
    try {
        // Inicializar FileSystem
        fs = new FileSystem(diskPath);
        if (!fs->isValid()) {
            throw std::runtime_error("FileSystem initialization failed");
        }
        std::cout << "[StorageNode] FileSystem initialized successfully" << std::endl;
        
        // Crear cliente para comunicarse con master
        masterClient = new UDPClient(masterServerIp, masterServerPort);
        std::cout << "[StorageNode] Master client created" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[StorageNode] Initialization error: " << e.what() << std::endl;
        throw;
    }

    auto& logger = LogManager::instance();
    try {
        logger.configureRemote(masterServerIp, masterServerPort, "StorageNode");
        logger.info("StorageNode configured to forward logs to SafeSpaceServer at " + masterServerIp + ":" + std::to_string(masterServerPort));
    } catch (const std::exception& ex) {
        std::cerr << "[StorageNode] Failed to configure SafeSpace log forwarding: " << ex.what() << std::endl;
    }
}

StorageNode::~StorageNode() {
    
    auto& logger = LogManager::instance();
    logger.info("StorageNode cerrándose - Estadísticas: Registros=" + 
            std::to_string(totalSensorRecords.load()) + 
            ", Consultas=" + std::to_string(totalQueries.load()) + 
            ", Errores=" + std::to_string(errorsCount.load()));
    std::cout << "[StorageNode] Shutting down..." << std::endl;
    
    // Detener thread de escucha
    listening.store(false);
    
    if (listenerThread.joinable()) {
        std::cout << "[StorageNode] Waiting for listener thread to finish..." << std::endl;
        listenerThread.join();
    }
    
    // Liberar cliente
    if (masterClient != nullptr) {
        delete masterClient;
        masterClient = nullptr;
        std::cout << "[StorageNode] Master client destroyed" << std::endl;
    }
    
    std::cout << "[StorageNode] Shutdown complete" << std::endl;
}

void StorageNode::start() {
    if (masterClient == nullptr) {
        throw std::runtime_error("[StorageNode] Cannot start: master client not initialized");
    }
    
    std::cout << "[StorageNode] Starting storage node: " << nodeId << std::endl;
    
    // Registrarse con el master
    registerWithMaster();
    
    // Iniciar thread para escuchar respuestas del master
    listening.store(true);
    listenerThread = std::thread(&StorageNode::listenMasterServerResponses, this);
    
    std::cout << "[StorageNode] Listener thread started" << std::endl;
    std::cout << "[StorageNode] Ready to receive sensor data" << std::endl;
    
    // Iniciar servidor UDP (bloqueante)
    try {
        serveBlocking();
    } catch (const std::exception &e) {
        std::cerr << "[StorageNode] Exception in serveBlocking(): " << e.what() << std::endl;
        try { auto& logger = LogManager::instance(); logger.error(std::string("StorageNode serveBlocking exception: ") + e.what()); } catch(...){}
    } catch (...) {
        std::cerr << "[StorageNode] Unknown exception in serveBlocking()" << std::endl;
        try { auto& logger = LogManager::instance(); logger.error("StorageNode serveBlocking unknown exception"); } catch(...){}
    }
}

void StorageNode::onReceive(const sockaddr_in& peer, const uint8_t* data,
                           ssize_t len, std::string& out_response) {
    std::string peerStr = sockaddrToString(peer);
    std::cout << "[StorageNode] Received " << len << " bytes from " 
              << peerStr << std::endl;
    
    if (len < 1) {
        std::cerr << "[StorageNode] Invalid message: too short" << std::endl;
        errorsCount++;
        return;
    }
    
    Response response;
    MessageType msgType = static_cast<MessageType>(data[0]);
    
    try {
        // Primero verificar por tipo de mensaje explícito
        switch (msgType) {
            case MessageType::QUERY_BY_DATE:
                std::cout << "[StorageNode] Query by date" << std::endl;
                response = handleQueryByDate(data, len);
                break;
                
            case MessageType::QUERY_BY_SENSOR:
                std::cout << "[StorageNode] Query by sensor" << std::endl;
                response = handleQueryBySensor(data, len);
                break;
                
            case MessageType::STORE_SENSOR_DATA:
                std::cout << "[StorageNode] Store sensor data" << std::endl;
                response = handleStoreSensorData(data, len);
                break;
                
            case MessageType::STORE_BITACORA:
                std::cout << "[StorageNode] Store bitácora" << std::endl;
                response = handleStoreBitacora(data, len);
                break;
                
            case MessageType::SENSOR_DATA: // GET_SENSOR_DATA_REQUEST
                response = handleGetSensorDataRequest(data, len);
                break;

            case MessageType::DELETE_SENSOR_DATA: // DELETE_SENSOR_DATA_REQUEST
                response = handleDeleteSensorDataRequest(data, len);
                break;
                
            default:
                // Si no es un tipo conocido, usar detección por longitud
                if (len == 17) {  // Consulta por fecha: [msgType][startTime(8)][endTime(8)]
                    std::cout << "[StorageNode] Detected QUERY_BY_DATE (legacy format)" << std::endl;
                    response = handleQueryByDate(data, len);
                    
                } else if (len == 18) {  // Consulta por sensor: [msgType][sensorId][startTime(8)][endTime(8)]
                    std::cout << "[StorageNode] Detected QUERY_BY_SENSOR (legacy format)" << std::endl;
                    response = handleQueryBySensor(data, len);
                    
                } else if (len >= 24 && len <= 100) {  // Guardar datos de sensores (25-100 bytes)
                    std::cout << "[StorageNode] Detected STORE_SENSOR_DATA" << std::endl;
                    response = handleStoreSensorData(data, len);
                    
                } else {
                    std::cout << "[StorageNode] Unknown message format (" << len 
                             << " bytes), using default behavior" << std::endl;
                    // Llamar al comportamiento por defecto de UDPServer (echo)
                    UDPServer::onReceive(peer, data, len, out_response);
                    return;
                }
        }
    } catch (const std::exception& e) {
        std::cerr << "[StorageNode] Error processing message: " << e.what() << std::endl;
        response.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ERROR);
        response.status = 1;
        errorsCount++;
    }
    
    // Convertir respuesta a string
    auto responseBytes = response.toBytes();
    out_response.assign(reinterpret_cast<const char*>(responseBytes.data()),
                       responseBytes.size());
}

Response StorageNode::handleGetSensorDataRequest(const uint8_t* data, ssize_t len) {
    Response resp;
    resp.msgId = 0x91; // GET_SENSOR_DATA_RESPONSE

    // Validate packet size: 1 + 2 + 16 = 19 bytes minimum
    if (len < 19) {
        std::cerr << "[StorageNode] Invalid GET request length: " << len << std::endl;
        resp.status = 1;
        return resp;
    }

    // Extract sensorId (big-endian)
    uint16_t sensorId = (data[1] << 8) | data[2];

    std::cout << "[StorageNode] GET_SENSOR_DATA_REQUEST for sensorId=" << sensorId << std::endl;

    // TODO: (Optional) Validate 16-byte token data+3 .. data+18

    // Query ALL historical entries for this sensor
    uint64_t start = 0;
    uint64_t end = UINT64_MAX;

    auto entries = querySensorDataById(sensorId, start, end);

    // Serialize all sensor entries
    for (const auto& sd : entries) {
        auto bytes = sensorDataToBytes(sd);
        resp.data.insert(resp.data.end(), bytes.begin(), bytes.end());
    }

    resp.status = 0;
    return resp;
}

Response StorageNode::handleStoreSensorDataRequest(const uint8_t* data, ssize_t len) {
  Response resp;
  resp.msgId = 0x93; // STORE_SENSOR_DATA_RESPONSE

  // Minimum size: 1 + 2 + 16 + 2 = 21 bytes BEFORE payload
  if (len < 21) {
    std::cerr << "[StorageNode] STORE request too short (" << len << " bytes)\n";
    resp.status = 1;
    return resp;
  }

  uint16_t sensorId = (data[1] << 8) | data[2];

  // Extract declared payload size
  uint16_t payloadSize = (data[19] << 8) | data[20];

  if (21 + payloadSize > len) {
    std::cerr << "[StorageNode] STORE payload mismatch: declared=" 
              << payloadSize << " received=" << (len - 21) << std::endl;
    resp.status = 1;
    return resp;
  }

  const uint8_t* payload = data + 21;

  std::cout << "[StorageNode] STORE_SENSOR_DATA_REQUEST for sensor=" << sensorId
            << " payloadSize=" << payloadSize << std::endl;

  // Convert payload to string
  std::string raw(reinterpret_cast<const char*>(payload), payloadSize);

  try {
    // Convert CSV into SensorData struct
    SensorData sd = stringToSensorData(raw);

    std::lock_guard<std::mutex> lock(fsMutex);
    bool ok = storeSensorDataToFS(sd);

    resp.status = ok ? 0 : 1;
  }
  catch (const std::exception& ex) {
    std::cerr << "[StorageNode] Error parsing STORE payload: " << ex.what() << std::endl;
    resp.status = 1;
  }

  return resp;
}

Response StorageNode::handleDeleteSensorDataRequest(const uint8_t* data, ssize_t len) {
  Response resp;
  resp.msgId = 0x95; // DELETE_SENSOR_DATA_RESPONSE

  if (len < 19) {
    std::cerr << "[StorageNode] DELETE request too short (" << len << " bytes)\n";
    resp.status = 1;
    return resp;
  }

  uint16_t sensorId = (data[1] << 8) | data[2];

  std::cout << "[StorageNode] DELETE_SENSOR_DATA_REQUEST for sensorId=" << sensorId << std::endl;

  bool removedAny = false;

  std::lock_guard<std::mutex> lock(fsMutex);

  for (const auto& entry : fs->getDirectory()) {
    std::string filename(entry.name);

    std::smatch m;
    std::regex pattern(R"(sensor_(\d+)_(\d+)\.dat)");

    if (std::regex_match(filename, m, pattern)) {
      uint16_t fileId = std::stoi(m[1]);
      if (fileId == sensorId) {
        std::cout << "[StorageNode] Removing file: " << filename << std::endl;
        fs->remove(filename);
        removedAny = true;
      }
    }
  }

  resp.status = removedAny ? 0 : 1;
  return resp;
}


Response StorageNode::handleQueryByDate(const uint8_t* data, ssize_t len) {
    Response resp;
    resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_SENSOR_HISTORY);
    
    // Formato: [msgType][startTime(8)][endTime(8)] = 17 bytes
    if (len < 17) {
        resp.status = 1;
        errorsCount++;
        return resp;
    }
    
    uint64_t startTime, endTime;
    std::memcpy(&startTime, data + 1, 8);
    std::memcpy(&endTime, data + 9, 8);
    startTime = be64toh(startTime);
    endTime = be64toh(endTime);
    
    std::cout << "[StorageNode] Querying from " << timestampToString(startTime)
              << " to " << timestampToString(endTime) << std::endl;
    
    std::lock_guard<std::mutex> lock(fsMutex);
    auto results = querySensorDataByDate(startTime, endTime);
    
    std::cout << "[StorageNode] Found " << results.size() << " records" << std::endl;
    
    // Serializar resultados
    for (const auto& record : results) {
        auto bytes = sensorDataToBytes(record);
        resp.data.insert(resp.data.end(), bytes.begin(), bytes.end());
    }
    
    resp.status = 0;
    totalQueries++;

    // Imprimir tabla de resultados - usar hora actual como placeholder
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
    
    // Imprimir tabla de resultados
    std::cout << "[StorageNode] Fecha | Distancia | Temperatura | Presión | Altitud | Presión nivel mar | Altitud real" << std::endl;
    for (const auto& record : results) {
        std::cout << timestampToString(currentTimeT) << " | "
                  << record.distance << " | "
                  << record.temperature << " | "
                  << record.pressure << " | "
                  << record.altitude << " | "
                  << record.sealevelPressure << " | "
                  << record.realAltitude << std::endl;
    }
    
    return resp;
}

Response StorageNode::handleQueryBySensor(const uint8_t* data, ssize_t len) {
    Response resp;
    resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_SENSOR_HISTORY);
    
    // Formato: [msgType][sensorId][startTime(8)][endTime(8)] = 18 bytes
    if (len < 18) {
        resp.status = 1;
        errorsCount++;
        return resp;
    }
    
    uint8_t sensorId = data[1];
    uint64_t startTime, endTime;
    std::memcpy(&startTime, data + 2, 8);
    std::memcpy(&endTime, data + 10, 8);
    startTime = be64toh(startTime);
    endTime = be64toh(endTime);
    
    std::cout << "[StorageNode] Querying sensor " << static_cast<int>(sensorId)
              << " from " << timestampToString(startTime)
              << " to " << timestampToString(endTime) << std::endl;
    
    std::lock_guard<std::mutex> lock(fsMutex);
    auto results = querySensorDataById(sensorId, startTime, endTime);
    
    std::cout << "[StorageNode] Found " << results.size() << " records" << std::endl;
    
    // Serializar resultados
    for (const auto& record : results) {
        auto bytes = sensorDataToBytes(record);
        resp.data.insert(resp.data.end(), bytes.begin(), bytes.end());
    }
    
    resp.status = 0;
    totalQueries++;

    // Imprimir tabla de resultados - usar hora actual como placeholder
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeT = std::chrono::system_clock::to_time_t(currentTime);
    
  // Imprimir tabla
    std::cout << "[StorageNode] Fecha | Datos del sensor" << std::endl;
    for (const auto& record : results) {
        std::cout << timestampToString(currentTimeT) << " | "
                  << "Dist:" << record.distance << "mm "
                  << "Temp:" << record.temperature << "°C "
                  << "Pres:" << record.pressure << "Pa "
                  << "Alt:" << record.altitude << "m "
                  << "PresMar:" << record.sealevelPressure << "Pa "
                  << "AltReal:" << record.realAltitude << "m" << std::endl;
    }
    
    return resp;
}

Response StorageNode::handleStoreSensorData(const uint8_t* data, ssize_t len) {
    Response resp;
    resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ACK);

    std::cout << "[StorageNode] handleStoreSensorData - len: " << len << std::endl;
    
    // Parsear datos del sensor (mínimo 24 bytes para 6 floats)
    if (len != sizeof(SensorData)) {  // 1 byte tipo + 24 bytes datos (6 floats)
        resp.status = 1;
        errorsCount++;
        std::cerr << "[StorageNode] Invalid sensor data length: " << len << std::endl;
        return resp;
    }
    
    try {
        std::cout << "[StorageNode] Converting bytes to SensorData..." << std::endl;
        SensorData sensorData = bytesToSensorData(data, len);


        std::cout << "[StorageNode] Storing SensorData to FS..." << std::endl;
        std::lock_guard<std::mutex> lock(fsMutex);
        bool success = storeSensorDataToFS(sensorData);
        
        if (success) {
            resp.status = 0;
            totalSensorRecords++;
            
             std::cout << "[StorageNode] Guardado - "
                      << "Dist:" << sensorData.distance << "mm | "
                      << "Temp:" << sensorData.temperature << "°C | "
                      << "Pres:" << sensorData.pressure << "Pa | "
                      << "Alt:" << sensorData.altitude << "m | "
                      << "PresMar:" << sensorData.sealevelPressure << "Pa | "
                      << "AltReal:" << sensorData.realAltitude << "m" << std::endl;
        } else {
            resp.status = 1;
            errorsCount++;
            std::cerr << "[StorageNode] Failed to store sensor data" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[StorageNode] Error storing sensor data: " << e.what() << std::endl;
        resp.status = 1;
        errorsCount++;
    }
    
    return resp;
}

Response StorageNode::handleStoreBitacora(const uint8_t* data, ssize_t len) {
    Response resp;
    resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ACK);
    
    if (len < 2) {
        resp.status = 1;
        errorsCount++;
        return resp;
    }
    
    try {
        // Extraer mensaje de bitácora
        std::string message(reinterpret_cast<const char*>(data + 1), len - 1);
        
        std::lock_guard<std::mutex> lock(fsMutex);
        
        // Archivo de bitácora
        std::string bitacoraFile = "bitacora.log";
        
        // Crear si no existe
        if (fs->find(bitacoraFile) < 0) {
            fs->create(bitacoraFile);
        }
        
        // Abrir, leer contenido actual, agregar nueva entrada
        fs->openFile(bitacoraFile);
        std::string currentLog = fs->read(bitacoraFile);
        
        // Agregar timestamp + mensaje
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::string entry = "[" + timestampToString(timestamp) + "] " + message + "\n";
        
        currentLog += entry;
        
        // Escribir de vuelta
        bool success = fs->write(bitacoraFile, currentLog);
        fs->closeFile(bitacoraFile);
        
        if (success) {
            resp.status = 0;
            std::cout << "[StorageNode] Bitácora: " << entry;
        } else {
            resp.status = 1;
            errorsCount++;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[StorageNode] Error storing bitácora: " << e.what() << std::endl;
        resp.status = 1;
        errorsCount++;
    }
    
    return resp;
}

bool StorageNode::storeSensorDataToFS(const SensorData& data) {
    // Usar segundos consistentemente
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    // Usar sensorId 0 por defecto
    uint8_t sensorId = 0;
    std::string filename = generateSensorFilename(timestamp, sensorId);
    
    std::cout << "[StorageNode] Creating file: " << filename 
              << " with timestamp: " << timestampToString(timestamp) << std::endl;
    
    // Crear archivo si no existe
    if (fs->find(filename) < 0) {
        if (fs->create(filename) < 0) {
            std::cerr << "[StorageNode] Failed to create file: " << filename << std::endl;
            auto& logger = LogManager::instance();
            try{
                logger.error("Failed to create file: " + filename);
            } catch (const std::exception& ex) {
                std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
            }
            return false;
        }
    } else {
        std::cout << "[StorageNode] File already exists: " << filename 
                  << ", data will be appended" << std::endl;
    }
    
    // Abrir archivo
    if (fs->openFile(filename) != 0) {
        std::cerr << "[StorageNode] Failed to open file: " << filename << std::endl;
        auto& logger = LogManager::instance();
        try{
            logger.error("Failed to open file: " + filename);
        } catch (const std::exception& ex) {
            std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
        }
        
        return false;
    }
    
    // Leer contenido existente (si hay)
    std::string existingContent = fs->read(filename);
    
    // Convertir SensorData a string para almacenar
    std::string dataString = sensorDataToString(data);
    
    // Si ya hay contenido, agregar nueva línea
    std::string contentToWrite;
    if (!existingContent.empty()) {
        contentToWrite = existingContent + "\n" + dataString;
        std::cout << "[StorageNode] Appending to existing file content" << std::endl;
    } else {
        contentToWrite = dataString;
        std::cout << "[StorageNode] Writing new file content" << std::endl;
    }
    
    // Escribir
    bool success = fs->write(filename, contentToWrite);
    
    // Cerrar
    fs->closeFile(filename);
    
    if (success) {
        std::cout << "[StorageNode] Successfully stored data in: " << filename << std::endl;
    }
    
    return success;
}

std::vector<SensorData> StorageNode::querySensorDataByDate(uint64_t startTime, uint64_t endTime) {
    std::vector<SensorData> results;

    std::cout << "[StorageNode] Searching files from " << timestampToString(startTime) 
              << " to " << timestampToString(endTime) << std::endl;

    // Recorremos las entradas del directorio virtual
    for (const auto& entry : fs->getDirectory()) {
        if (entry.inode_id == 0) continue; // entrada vacía

        std::string filename(entry.name);
        std::cout << "[StorageNode] Checking file: " << filename << std::endl;

        // Coincide con formato sensor_<id>_<timestamp>.dat
        std::smatch match;
        std::regex pattern(R"(sensor_(\d+)_(\d+)\.dat)");
        if (!std::regex_match(filename, match, pattern)) {
            continue;
        }

        uint64_t timestamp = std::stoull(match[2].str());
        std::cout << "[StorageNode] File " << filename << " has timestamp: " << timestampToString(timestamp) << std::endl;
        
        if (timestamp < startTime || timestamp > endTime) {
            std::cout << "[StorageNode] File " << filename << " outside time range, skipping" << std::endl;
            continue;
        }

        if (fs->openFile(filename) != 0) {
            std::cout << "[StorageNode] Failed to open file: " << filename << std::endl;
            continue;
        }
        
        std::string raw = fs->read(filename);
        fs->closeFile(filename);

        std::cout << "[StorageNode] Raw data from " << filename << ": " << raw << std::endl;

        // Procesar múltiples líneas si existen
        std::istringstream rawStream(raw);
        std::string line;
        int lineCount = 0;
        
        while (std::getline(rawStream, line)) {
            if (line.empty()) continue;
            
            lineCount++;
            std::cout << "[StorageNode] Processing line " << lineCount << ": " << line << std::endl;
            
            try {
                SensorData sd = stringToSensorData(line);
                results.push_back(sd);
                std::cout << "[StorageNode] Successfully parsed line " << lineCount 
                          << ": Dist=" << sd.distance << " Temp=" << sd.temperature << std::endl;
                          
            } catch (const std::exception& e) {
                std::cerr << "[StorageNode] Error parsing line " << lineCount 
                          << " in file " << filename << ": " << e.what() << "\n";
            }
        }
        
        std::cout << "[StorageNode] Processed " << lineCount << " lines from " << filename << std::endl;
    }

    std::cout << "[StorageNode] " << results.size() << " registers found within date range.\n";
    return results;
}

std::vector<SensorData> StorageNode::querySensorDataById(uint8_t sensorId, uint64_t startTime, uint64_t endTime) {
    std::vector<SensorData> results;

    std::cout << "[StorageNode] Searching for sensor " << static_cast<int>(sensorId)
              << " from " << timestampToString(startTime) 
              << " to " << timestampToString(endTime) << std::endl;

    for (const auto& entry : fs->getDirectory()) {
        if (entry.inode_id == 0) continue;

        std::string filename(entry.name);
        std::cout << "[StorageNode] Checking file: " << filename << std::endl;

        std::smatch match;
        std::regex pattern(R"(sensor_(\d+)_(\d+)\.dat)");
        if (!std::regex_match(filename, match, pattern)) {
            std::cout << "[StorageNode] File " << filename << " doesn't match pattern, skipping" << std::endl;
            continue;
        }

        uint8_t fileId = static_cast<uint8_t>(std::stoi(match[1].str()));
        uint64_t timestamp = std::stoull(match[2].str());

        std::cout << "[StorageNode] File " << filename << " has sensorId: " << static_cast<int>(fileId)
                  << " and timestamp: " << timestampToString(timestamp) << std::endl;

        // Filtramos por ID y rango de tiempo
        if (fileId != sensorId) {
            std::cout << "[StorageNode] Sensor ID mismatch (expected " << static_cast<int>(sensorId)
                      << ", got " << static_cast<int>(fileId) << "), skipping" << std::endl;
            continue;
        }
        if (timestamp < startTime || timestamp > endTime) {
            std::cout << "[StorageNode] File " << filename << " outside time range, skipping" << std::endl;
            continue;
        }

        if (fs->openFile(filename) != 0) {
            std::cout << "[StorageNode] Failed to open file: " << filename << std::endl;
            continue;
        }
        
        std::string raw = fs->read(filename);
        fs->closeFile(filename);

        std::cout << "[StorageNode] Raw data from " << filename << ": " << raw << std::endl;

        // Procesar múltiples líneas si existen
        std::istringstream rawStream(raw);
        std::string line;
        int lineCount = 0;
        
        while (std::getline(rawStream, line)) {
            if (line.empty()) continue;
            
            lineCount++;
            std::cout << "[StorageNode] Processing line " << lineCount << ": " << line << std::endl;
            
            try {
                SensorData sd = stringToSensorData(line);
                results.push_back(sd);
                std::cout << "[StorageNode] Successfully parsed line " << lineCount 
                          << ": Dist=" << sd.distance << " Temp=" << sd.temperature << std::endl;
                          
            } catch (const std::exception& e) {
                std::cerr << "[StorageNode] Error parsing line " << lineCount 
                          << " in file " << filename << ": " << e.what() << "\n";
            }
        }
        
        std::cout << "[StorageNode] Processed " << lineCount << " lines from " << filename << std::endl;
    }

    std::cout << "[StorageNode] " << results.size()
              << " registers found for the sensor " << (int)sensorId << ".\n";

    return results;
}

void StorageNode::registerWithMaster() {
    std::cout << "[StorageNode] Registering with master server..." << std::endl;
    
    auto& logger = LogManager::instance();
    try {
        logger.info("Attempting to register StorageNode with master server at " + masterServerIp + ":" + std::to_string(masterServerPort));
    } catch (const std::exception& ex) {
        std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
    }

    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(MessageType::REGISTER_NODE));
    msg.push_back(static_cast<uint8_t>(nodeId.length()));
    msg.insert(msg.end(), nodeId.begin(), nodeId.end());
    
    std::cout << "[StorageNode] Registration prepared" << std::endl;
    logger.info("[StorageNode] Registration with master server prepared successfully");
}

void StorageNode::sendHeartbeat() {
    std::vector<uint8_t> msg;
    msg.push_back(static_cast<uint8_t>(MessageType::HEARTBEAT));
    msg.push_back(static_cast<uint8_t>(nodeId.length()));
    msg.insert(msg.end(), nodeId.begin(), nodeId.end());
    auto& logger = LogManager::instance();
    try {
        logger.info("Sending heartbeat to master server - node: " + nodeId);
    } catch (const std::exception& ex) {
        std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
    }
}

void StorageNode::listenMasterServerResponses() {
    std::cout << "[StorageNode] Listener thread running" << std::endl;
    
    auto& logger = LogManager::instance();
    try {
        logger.info("Listener thread started - monitoring every 30s");
    } catch (const std::exception& ex) {
        std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
    }
    
    while (listening.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(30));
        sendHeartbeat();
    }
    
    std::cout << "[StorageNode] Listener thread stopped" << std::endl;
    try {
        logger.info("Listener thread stopped");
    } catch (const std::exception& ex) {
        std::cerr << "[StorageNode] Logging error: " << ex.what() << std::endl;
    }
}

std::string StorageNode::generateSensorFilename(uint64_t timestamp, uint8_t sensorId) const {
    std::ostringstream oss;
    oss << "sensor_" << static_cast<int>(sensorId) 
        << "_" << timestamp << ".dat";
    return oss.str();
}

std::string StorageNode::generateDateIndexFilename(uint64_t timestamp) const {
    std::ostringstream oss;
    oss << "index_" << (timestamp / 86400) << ".idx";
    return oss.str();
}

std::string StorageNode::timestampToString(uint64_t timestamp) const {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string StorageNode::sockaddrToString(const sockaddr_in& addr) const {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
}

StorageNode::Stats StorageNode::getStats() const {
    std::lock_guard<std::mutex> lock(fsMutex);
    Stats stats;
    stats.totalSensorRecords = totalSensorRecords.load();
    stats.totalQueries = totalQueries.load();
    stats.errorsCount = errorsCount.load();
    return stats;
}