#ifndef STORAGENODE_H
#define STORAGENODE_H

#include "../interfaces/UDPServer.h"
#include "../interfaces/UDPClient.h"
#include "../../model/filesystem/FileSystem.h"
#include "../../common/LogManager.h"
#include "../../model/structures/sensordata.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>
#include <thread>
#include <cstdint>
#include <atomic>

// Tipos de mensajes del protocolo
enum class MessageType : uint8_t {
    // Consultas
    QUERY_BY_DATE = 0x01,           // Consultar por fecha (2 bytes)
    QUERY_BY_SENSOR = 0x02,         // Consultar por sensor ID (15 bytes)
    SENSOR_DATA = 0x90,
    DELETE_SENSOR_DATA = 0x94,

    // Almacenamiento
    STORE_SENSOR_DATA = 0x10,       // Guardar datos de sensores (50 bytes)
    STORE_BITACORA = 0x11,          // Guardar bitácora

    // Respuestas
    RESPONSE_SENSOR_DATA = 0x93,    // Respuesta con datos de sensores
    RESONSE_DELETE_SENSOR_DATA = 0x95,
    RESPONSE_SENSOR_HISTORY = 0x21, // Respuesta con histórico
    RESPONSE_ACK = 0x22,            // Confirmación
    RESPONSE_ERROR = 0x23,          // Error

    // Registro y descubrimiento
    REGISTER_NODE = 0x30,           // Registrar nodo en master
    HEARTBEAT = 0x31                // Mantener conexión
};

// Estructura para respuestas
struct Response {
    uint8_t msgId;
    uint8_t status;
    std::vector<uint8_t> data;

    std::vector<uint8_t> toBytes() const;
};

class StorageNode: public UDPServer {
 public:
    StorageNode(uint16_t storagePort, const std::string& masterServerIp,
                uint16_t masterServerPort, const std::string& nodeId,
                const std::string& diskPath, size_t bufsize = 65536);
    ~StorageNode() override;

    void start();

   // Obtener estadísticas
   struct Stats {
      size_t totalSensorRecords;
      size_t totalQueries;
      size_t errorsCount;
      size_t filesStored;
   };

   Stats getStats() const;
   
       // Método público para pruebas
    void testReceive(const uint8_t* data, ssize_t len, std::string& out_response) {
        sockaddr_in testPeer{};
        onReceive(testPeer, data, len, out_response);
    }


 private:
    // Cliente para comunicarse con el master
    UDPClient* masterClient;
    std::string masterServerIp;
    uint16_t masterServerPort;
    std::string nodeId;
    std::string diskPath;

    FileSystem* fs;
    mutable std::mutex fsMutex;

    // Thread para escuchar respuestas del master
    std::thread listenerThread;
    std::atomic<bool> listening;

    // Estadísticas
    std::atomic<size_t> totalSensorRecords;
    std::atomic<size_t> totalQueries;
    std::atomic<size_t> errorsCount;

    // Métodos privados
    void listenMasterServerResponses();
    void registerWithMaster();
    void sendHeartbeat();

    // Manejadores de mensajes
    Response handleQueryByDate(const uint8_t* data, ssize_t len);
    Response handleQueryBySensor(const uint8_t* data, ssize_t len);
    Response handleStoreSensorData(const uint8_t* data, ssize_t len);
    Response handleStoreBitacora(const uint8_t* data, ssize_t len, const std::string& peerLabel);
    Response handleStoreBitacora(const uint8_t* data, ssize_t len);
    Response handleGetSensorDataRequest(const uint8_t* data, ssize_t len);
    Response handleStoreSensorDataRequest(const uint8_t* data, ssize_t len);
    Response handleDeleteSensorDataRequest(const uint8_t* data, ssize_t len);

    // Utilidades
    std::string sockaddrToString(const sockaddr_in& addr) const;
    std::string generateSensorFilename(uint64_t timestamp, uint8_t sensorId) const;
    std::string generateDateIndexFilename(uint64_t timestamp) const;
    std::string timestampToString(uint64_t timestamp) const;

    // Almacenamiento y consulta
    bool storeSensorDataToFS(const SensorData& data);
    std::vector<SensorData> querySensorDataByDate(uint64_t startTime, uint64_t endTime);
    std::vector<SensorData> querySensorDataById(uint8_t sensorId, uint64_t startTime, uint64_t endTime);

    std::vector<uint8_t> sensorDataToBytes(const SensorData& data) const;
    SensorData bytesToSensorData(const uint8_t* data, size_t len) const;
    std::string sensorDataToString(const SensorData& data) const;
    SensorData stringToSensorData(const std::string& str) const;

 protected:
    /**
     * Maneja mensajes recibidos del master.
     */
   void onReceive(const sockaddr_in& peer, const uint8_t* data,
                   ssize_t len, std::string& out_response) override;
};
#endif // STORAGENODE_H