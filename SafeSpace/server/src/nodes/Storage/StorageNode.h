#ifndef STORAGENODE_H
#define STORAGENODE_H

#include "../interfaces/UDPServer.h"
#include "../interfaces/UDPClient.h"
#include "../../model/filesystem/FileSystem.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <memory>

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
                const std::string& diskPath, size_t bufsize = 65536);
    ~StorageNode() override;

    void start();

    void listenMasterServerResponses();

 private:
    UDPClient* masterClient;
    std::string masterServerIP;
    std::string nodeId;
    std::string diskPath;
    FileSystem& fileSystem;

 protected:
    /**
     * Maneja mensajes recibidos del master.
     */
    void onReceive(const sockaddr_in& peer, const uint8_t* data, 
                   ssize_t len, std::string& out_response) override;  
};
#endif // STORAGENODE_H