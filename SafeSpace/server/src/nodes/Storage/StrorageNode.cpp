// #include "StorageNode.h"
// #include <iostream>
// #include <iomanip>
// #include <sstream>
// #include <cstring>
// #include <arpa/inet.h>
// #include <chrono>
// #include <unistd.h>
//
// const size_t BUFFER_SIZE = 2048;
//
// std::vector<uint8_t> SensorData::toBytes() const {
//     std::vector<uint8_t> bytes;
//     bytes.reserve(24);
//
//     uint64_t ts_net = htobe64(timestamp);
//     bytes.insert(bytes.end(), (uint8_t*)&ts_net, (uint8_t*)&ts_net + 8);
//
//     uint16_t dist_net = htons(distance);
//     bytes.insert(bytes.end(), (uint8_t*)&dist_net, (uint8_t*)&dist_net + 2);
//
//     bytes.push_back(movement);
//
//     int16_t temp_net = htons(temperature);
//     bytes.insert(bytes.end(), (uint8_t*)&temp_net, (uint8_t*)&temp_net + 2);
//
//     uint16_t uv_net = htons(uv);
//     bytes.insert(bytes.end(), (uint8_t*)&uv_net, (uint8_t*)&uv_net + 2);
//
//     uint16_t mic_net = htons(microphone);
//     bytes.insert(bytes.end(), (uint8_t*)&mic_net, (uint8_t*)&mic_net + 2);
//
//     bytes.push_back(leds);
//
//     bytes.push_back(buzzer);
//
//     uint16_t luz_net = htons(light);
//     bytes.insert(bytes.end(), (uint8_t*)&luz_net, (uint8_t*)&luz_net + 2);
//
//     bytes.push_back(sensorId);
//
//     return bytes;
// }
//
// SensorData SensorData::fromBytes(const uint8_t* data, size_t len) {
//     if (len < 24) {
//         throw std::runtime_error("Invalid sensor data length");
//     }
//
//     SensorData sd;
//     size_t offset = 0;
//
//     // timestamp
//     uint64_t ts_net;
//     std::memcpy(&ts_net, data + offset, 8);
//     sd.timestamp = be64toh(ts_net);
//     offset += 8;
//
//     // distancia
//     uint16_t dist_net;
//     std::memcpy(&dist_net, data + offset, 2);
//     sd.distance = ntohs(dist_net);
//     offset += 2;
//
//     sd.movement = data[offset++];
//
//     int16_t temp_net;
//     std::memcpy(&temp_net, data + offset, 2);
//     sd.temperature = ntohs(temp_net);
//     offset += 2;
//
//     // uv
//     uint16_t uv_net;
//     std::memcpy(&uv_net, data + offset, 2);
//     sd.uv = ntohs(uv_net);
//     offset += 2;
//
//     // microfono
//     uint16_t mic_net;
//     std::memcpy(&mic_net, data + offset, 2);
//     sd.microphone = ntohs(mic_net);
//     offset += 2;
//
//     // leds
//     sd.leds = data[offset++];
//
//     // buzzer
//     sd.buzzer = data[offset++];
//
//     // luz
//     uint16_t luz_net;
//     std::memcpy(&luz_net, data + offset, 2);
//     sd.light = ntohs(luz_net);
//     offset += 2;
//
//     // sensorId
//     sd.sensorId = data[offset++];
//
//     return sd;
// }
//
// std::vector<uint8_t> Response::toBytes() const {
//     std::vector<uint8_t> bytes;
//     bytes.reserve(2 + data.size());
//     bytes.push_back(msgId);
//     bytes.push_back(status);
//     bytes.insert(bytes.end(), data.begin(), data.end());
//     return bytes;
// }
//
// StorageNode::StorageNode(uint16_t storagePort, const std::string& masterServerIp,
//                          uint16_t masterServerPort, const std::string& nodeId,
//                          const std::string& diskPath, size_t bufsize)
//     : UDPServer(storagePort, bufsize),
//       masterClient(nullptr),
//       masterServerIp(masterServerIp),
//       masterServerPort(masterServerPort),
//       nodeId(nodeId),
//       diskPath(diskPath),
//       listening(false),
//       totalSensorRecords(0),
//       totalQueries(0),
//       errorsCount(0)
// {
//     std::cout << "[StorageNode] Initializing node: " << nodeId << std::endl;
//     std::cout << "[StorageNode] Storage port: " << storagePort << std::endl;
//     std::cout << "[StorageNode] Master server: " << masterServerIp
//               << ":" << masterServerPort << std::endl;
//     std::cout << "[StorageNode] Disk path: " << diskPath << std::endl;
//
//     try {
//         // Inicializar FileSystem
//         fs = new FileSystem(diskPath);
//         if (!fs->isValid()) {
//             throw std::runtime_error("FileSystem initialization failed");
//         }
//         std::cout << "[StorageNode] FileSystem initialized successfully" << std::endl;
//
//         // Crear cliente para comunicarse con master
//         masterClient = new UDPClient(masterServerIp, masterServerPort);
//         std::cout << "[StorageNode] Master client created" << std::endl;
//
//     } catch (const std::exception& e) {
//         std::cerr << "[StorageNode] Initialization error: " << e.what() << std::endl;
//         throw;
//     }
// }
//
// StorageNode::~StorageNode() {
//     std::cout << "[StorageNode] Shutting down..." << std::endl;
//
//     // Detener thread de escucha
//     listening.store(false);
//
//     if (listenerThread.joinable()) {
//         std::cout << "[StorageNode] Waiting for listener thread to finish..." << std::endl;
//         listenerThread.join();
//     }
//
//     // Liberar cliente
//     if (masterClient != nullptr) {
//         delete masterClient;
//         masterClient = nullptr;
//         std::cout << "[StorageNode] Master client destroyed" << std::endl;
//     }
//
//     std::cout << "[StorageNode] Shutdown complete" << std::endl;
// }
//
// void StorageNode::start() {
//     if (masterClient == nullptr) {
//         throw std::runtime_error("[StorageNode] Cannot start: master client not initialized");
//     }
//
//     std::cout << "[StorageNode] Starting storage node: " << nodeId << std::endl;
//
//     // Registrarse con el master
//     registerWithMaster();
//
//     // Iniciar thread para escuchar respuestas del master
//     listening.store(true);
//     listenerThread = std::thread(&StorageNode::listenMasterServerResponses, this);
//
//     std::cout << "[StorageNode] Listener thread started" << std::endl;
//     std::cout << "[StorageNode] Ready to receive sensor data" << std::endl;
//
//     // Iniciar servidor UDP (bloqueante)
//     serveBlocking();
// }
//
// void StorageNode::onReceive(const sockaddr_in& peer, const uint8_t* data,
//                            ssize_t len, std::string& out_response) {
//     std::string peerStr = sockaddrToString(peer);
//     std::cout << "[StorageNode] Received " << len << " bytes from "
//               << peerStr << std::endl;
//
//     if (len < 1) {
//         std::cerr << "[StorageNode] Invalid message: too short" << std::endl;
//         errorsCount++;
//         return;
//     }
//
//     Response response;
//     MessageType msgType = static_cast<MessageType>(data[0]);
//
//     try {
//         // Detectar tipo de mensaje por longitud (backward compatibility)
//         if (len == 17) {  // Consulta por fecha: [msgType][startTime(8)][endTime(8)]
//             std::cout << "[StorageNode] Detected QUERY_BY_DATE (legacy format)" << std::endl;
//             response = handleQueryByDate(data, len);
//
//         } else if (len == 18) {  // Consulta por sensor: [msgType][sensorId][startTime(8)][endTime(8)]
//             std::cout << "[StorageNode] Detected QUERY_BY_SENSOR (legacy format)" << std::endl;
//             response = handleQueryBySensor(data, len);
//
//         } else if (len >= 24) {  // Guardar datos de sensores (mínimo 24 bytes)
//             std::cout << "[StorageNode] Detected STORE_SENSOR_DATA" << std::endl;
//             response = handleStoreSensorData(data, len);
//
//         } else {
//             // Usar el tipo de mensaje explícito
//             switch (msgType) {
//                 case MessageType::QUERY_BY_DATE:
//                     std::cout << "[StorageNode] Query by date" << std::endl;
//                     response = handleQueryByDate(data, len);
//                     break;
//
//                 case MessageType::QUERY_BY_SENSOR:
//                     std::cout << "[StorageNode] Query by sensor" << std::endl;
//                     response = handleQueryBySensor(data, len);
//                     break;
//
//                 case MessageType::STORE_SENSOR_DATA:
//                     std::cout << "[StorageNode] Store sensor data" << std::endl;
//                     response = handleStoreSensorData(data, len);
//                     break;
//
//                 case MessageType::STORE_BITACORA:
//                     std::cout << "[StorageNode] Store bitácora" << std::endl;
//                     response = handleStoreBitacora(data, len);
//                     break;
//
//                 default:
//                     std::cout << "[StorageNode] Unknown message format (" << len
//                              << " bytes), using default behavior" << std::endl;
//                     // Llamar al comportamiento por defecto de UDPServer (echo)
//                     UDPServer::onReceive(peer, data, len, out_response);
//                     return;
//             }
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "[StorageNode] Error processing message: " << e.what() << std::endl;
//         response.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ERROR);
//         response.status = 1;
//         errorsCount++;
//     }
//
//     // Convertir respuesta a string
//     auto responseBytes = response.toBytes();
//     out_response.assign(reinterpret_cast<const char*>(responseBytes.data()),
//                        responseBytes.size());
// }
//
// Response StorageNode::handleQueryByDate(const uint8_t* data, ssize_t len) {
//     Response resp;
//     resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_SENSOR_HISTORY);
//
//     // Formato: [msgType][startTime(8)][endTime(8)] = 17 bytes
//     if (len < 17) {
//         resp.status = 1;
//         errorsCount++;
//         return resp;
//     }
//
//     uint64_t startTime, endTime;
//     std::memcpy(&startTime, data + 1, 8);
//     std::memcpy(&endTime, data + 9, 8);
//     startTime = be64toh(startTime);
//     endTime = be64toh(endTime);
//
//     std::cout << "[StorageNode] Querying from " << timestampToString(startTime)
//               << " to " << timestampToString(endTime) << std::endl;
//
//     std::lock_guard<std::mutex> lock(fsMutex);
//     auto results = querySensorDataByDate(startTime, endTime);
//
//     std::cout << "[StorageNode] Found " << results.size() << " records" << std::endl;
//
//     // Serializar resultados
//     for (const auto& record : results) {
//         auto bytes = record.toBytes();
//         resp.data.insert(resp.data.end(), bytes.begin(), bytes.end());
//     }
//
//     resp.status = 0;
//     totalQueries++;
//
//     // Imprimir tabla de resultados
//     std::cout << "[StorageNode] Fecha | Distancia | Movimiento | Temperatura | UV | Micrófono | Leds | Buzzer bit | Luz" << std::endl;
//     for (const auto& record : results) {
//         std::cout << timestampToString(record.timestamp) << " | "
//                   << record.distance << " | "
//                   << static_cast<int>(record.movement) << " | "
//                   << (record.temperature / 100.0) << " | "
//                   << record.uv << " | "
//                   << record.microphone << " | "
//                   << static_cast<int>(record.leds) << " | "
//                   << static_cast<int>(record.buzzer) << " | "
//                   << record.light << std::endl;
//     }
//
//     return resp;
// }
//
// Response StorageNode::handleQueryBySensor(const uint8_t* data, ssize_t len) {
//     Response resp;
//     resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_SENSOR_HISTORY);
//
//     // Formato: [msgType][sensorId][startTime(8)][endTime(8)] = 18 bytes
//     if (len < 18) {
//         resp.status = 1;
//         errorsCount++;
//         return resp;
//     }
//
//     uint8_t sensorId = data[1];
//     uint64_t startTime, endTime;
//     std::memcpy(&startTime, data + 2, 8);
//     std::memcpy(&endTime, data + 10, 8);
//     startTime = be64toh(startTime);
//     endTime = be64toh(endTime);
//
//     std::cout << "[StorageNode] Querying sensor " << static_cast<int>(sensorId)
//               << " from " << timestampToString(startTime)
//               << " to " << timestampToString(endTime) << std::endl;
//
//     std::lock_guard<std::mutex> lock(fsMutex);
//     auto results = querySensorDataById(sensorId, startTime, endTime);
//
//     std::cout << "[StorageNode] Found " << results.size() << " records" << std::endl;
//
//     // Serializar resultados
//     for (const auto& record : results) {
//         auto bytes = record.toBytes();
//         resp.data.insert(resp.data.end(), bytes.begin(), bytes.end());
//     }
//
//     resp.status = 0;
//     totalQueries++;
//
//     // Imprimir tabla
//     std::cout << "[StorageNode] Fecha | Dato" << std::endl;
//     for (const auto& record : results) {
//         std::cout << timestampToString(record.timestamp) << " | "
//                   << "Dist:" << record.distance << "mm "
//                   << "Mov:" << static_cast<int>(record.movement) << " "
//                   << "Temp:" << (record.temperature / 100.0) << "°C "
//                   << "UV:" << record.uv << " "
//                   << "Mic:" << record.microphone << " "
//                   << "Luz:" << record.light << std::endl;
//     }
//
//     return resp;
// }
//
// Response StorageNode::handleStoreSensorData(const uint8_t* data, ssize_t len) {
//     Response resp;
//     resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ACK);
//
//     // Parsear datos del sensor (mínimo 24 bytes + 1 byte tipo)
//     if (len < 25) {
//         resp.status = 1;
//         errorsCount++;
//         std::cerr << "[StorageNode] Invalid sensor data length: " << len << std::endl;
//         return resp;
//     }
//
//     try {
//         SensorData sensorData = SensorData::fromBytes(data + 1, len - 1);
//
//         std::lock_guard<std::mutex> lock(fsMutex);
//         bool success = storeSensorDataToFS(sensorData);
//
//         if (success) {
//             resp.status = 0;
//             totalSensorRecords++;
//
//             std::cout << "[StorageNode] Guardado - "
//                       << timestampToString(sensorData.timestamp) << " | "
//                       << sensorData.distance << " | "
//                       << static_cast<int>(sensorData.movement) << " | "
//                       << (sensorData.temperature / 100.0) << " | "
//                       << sensorData.uv << " | "
//                       << sensorData.microphone << " | "
//                       << static_cast<int>(sensorData.leds) << " | "
//                       << static_cast<int>(sensorData.buzzer) << " | "
//                       << sensorData.light << std::endl;
//         } else {
//             resp.status = 1;
//             errorsCount++;
//             std::cerr << "[StorageNode] Failed to store sensor data" << std::endl;
//         }
//     } catch (const std::exception& e) {
//         std::cerr << "[StorageNode] Error storing sensor data: " << e.what() << std::endl;
//         resp.status = 1;
//         errorsCount++;
//     }
//
//     return resp;
// }
//
// Response StorageNode::handleStoreBitacora(const uint8_t* data, ssize_t len) {
//     Response resp;
//     resp.msgId = static_cast<uint8_t>(MessageType::RESPONSE_ACK);
//
//     if (len < 2) {
//         resp.status = 1;
//         errorsCount++;
//         return resp;
//     }
//
//     try {
//         // Extraer mensaje de bitácora
//         std::string message(reinterpret_cast<const char*>(data + 1), len - 1);
//
//         std::lock_guard<std::mutex> lock(fsMutex);
//
//         // Archivo de bitácora
//         std::string bitacoraFile = "bitacora.log";
//
//         // Crear si no existe
//         if (fs->find(bitacoraFile) < 0) {
//             fs->create(bitacoraFile);
//         }
//
//         // Abrir, leer contenido actual, agregar nueva entrada
//         fs->openFile(bitacoraFile);
//         std::string currentLog = fs->read(bitacoraFile);
//
//         // Agregar timestamp + mensaje
//         auto now = std::chrono::system_clock::now();
//         auto timestamp = std::chrono::system_clock::to_time_t(now);
//         std::string entry = "[" + timestampToString(timestamp) + "] " + message + "\n";
//
//         currentLog += entry;
//
//         // Escribir de vuelta
//         bool success = fs->write(bitacoraFile, currentLog);
//         fs->closeFile(bitacoraFile);
//
//         if (success) {
//             resp.status = 0;
//             std::cout << "[StorageNode] Bitácora: " << entry;
//         } else {
//             resp.status = 1;
//             errorsCount++;
//         }
//
//     } catch (const std::exception& e) {
//         std::cerr << "[StorageNode] Error storing bitácora: " << e.what() << std::endl;
//         resp.status = 1;
//         errorsCount++;
//     }
//
//     return resp;
// }
//
// bool StorageNode::storeSensorDataToFS(const SensorData& data) {
//     // Generar nombre de archivo basado en timestamp y sensor
//     std::string filename = generateSensorFilename(data.timestamp, data.sensorId);
//
//     // Crear archivo si no existe
//     if (fs->find(filename) < 0) {
//         if (fs->create(filename) < 0) {
//             std::cerr << "[StorageNode] Failed to create file: " << filename << std::endl;
//             return false;
//         }
//     }
//
//     // Abrir archivo
//     if (fs->openFile(filename) != 0) {
//         std::cerr << "[StorageNode] Failed to open file: " << filename << std::endl;
//         return false;
//     }
//
//     // Serializar datos del sensor
//     auto bytes = data.toBytes();
//     std::string dataStr(bytes.begin(), bytes.end());
//
//     // Escribir
//     bool success = fs->write(filename, dataStr);
//
//     // Cerrar
//     fs->closeFile(filename);
//
//     return success;
// }
//
// std::vector<SensorData> StorageNode::querySensorDataByDate(uint64_t startTime, uint64_t endTime) {
//     std::vector<SensorData> results;
//
//     // TODO: Implementar búsqueda eficiente con índices
//     // Por ahora, esta es una implementación simple
//
//     std::cout << "[StorageNode] Query by date not fully implemented yet" << std::endl;
//
//     return results;
// }
//
// std::vector<SensorData> StorageNode::querySensorDataById(uint8_t sensorId, uint64_t startTime, uint64_t endTime) {
//     std::vector<SensorData> results;
//
//     // TODO: Implementar búsqueda por sensor ID en rango de tiempo
//
//     std::cout << "[StorageNode] Query by sensor ID not fully implemented yet" << std::endl;
//
//     return results;
// }
//
// void StorageNode::registerWithMaster() {
//     std::cout << "[StorageNode] Registering with master server..." << std::endl;
//
//     std::vector<uint8_t> msg;
//     msg.push_back(static_cast<uint8_t>(MessageType::REGISTER_NODE));
//     msg.push_back(static_cast<uint8_t>(nodeId.length()));
//     msg.insert(msg.end(), nodeId.begin(), nodeId.end());
//
//     std::cout << "[StorageNode] Registration prepared" << std::endl;
// }
//
// void StorageNode::sendHeartbeat() {
//     std::vector<uint8_t> msg;
//     msg.push_back(static_cast<uint8_t>(MessageType::HEARTBEAT));
//     msg.push_back(static_cast<uint8_t>(nodeId.length()));
//     msg.insert(msg.end(), nodeId.begin(), nodeId.end());
// }
//
// void StorageNode::listenMasterServerResponses() {
//     std::cout << "[StorageNode] Listener thread running" << std::endl;
//
//     while (listening.load()) {
//         std::this_thread::sleep_for(std::chrono::seconds(30));
//         sendHeartbeat();
//     }
//
//     std::cout << "[StorageNode] Listener thread stopped" << std::endl;
// }
//
// std::string StorageNode::generateSensorFilename(uint64_t timestamp, uint8_t sensorId) const {
//     std::ostringstream oss;
//     oss << "sensor_" << static_cast<int>(sensorId)
//         << "_" << timestamp << ".dat";
//     return oss.str();
// }
//
// std::string StorageNode::generateDateIndexFilename(uint64_t timestamp) const {
//     std::ostringstream oss;
//     oss << "index_" << (timestamp / 86400) << ".idx";
//     return oss.str();
// }
//
// std::string StorageNode::timestampToString(uint64_t timestamp) const {
//     std::time_t time = static_cast<std::time_t>(timestamp);
//     std::tm* tm = std::localtime(&time);
//     std::ostringstream oss;
//     oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
//     return oss.str();
// }
//
// std::string StorageNode::sockaddrToString(const sockaddr_in& addr) const {
//     char ip[INET_ADDRSTRLEN];
//     inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
//     return std::string(ip) + ":" + std::to_string(ntohs(addr.sin_port));
// }
//
// StorageNode::Stats StorageNode::getStats() const {
//     std::lock_guard<std::mutex> lock(fsMutex);
//     Stats stats;
//     stats.totalSensorRecords = totalSensorRecords.load();
//     stats.totalQueries = totalQueries.load();
//     stats.errorsCount = errorsCount.load();
//     return stats;
// }