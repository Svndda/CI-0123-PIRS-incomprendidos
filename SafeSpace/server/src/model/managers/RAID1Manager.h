// RAID1Manager.h
#ifndef RAID1MANAGER_H
#define RAID1MANAGER_H

#include <string>
#include <mutex>
#include <memory>
#include <stdexcept>

class FileSystem; // Forward declaration

enum class DiskStatus {
    OPERATIONAL,
    FAILED,
    DEGRADED
};

class RAID1Manager {
private:
    FileSystem& fileSystem;
    
    // Nombres de los discos
    std::string primaryDisk;
    std::string mirrorDisk;
    
    // Estado de los discos
    DiskStatus primaryStatus;
    DiskStatus mirrorStatus;
    
    // Control de concurrencia
    mutable std::mutex diskMutex;
    
    // Disco activo para lecturas
    std::string activeDisk;
    
    // Métodos privados auxiliares
    bool writeToDisk(const std::string& diskName, const std::string& data);
    std::string readFromDisk(const std::string& diskName);
    void checkDiskHealth(const std::string& diskName);
    void switchToMirror();
    void switchToPrimary();
    
public:
    RAID1Manager(FileSystem& fs, 
                 const std::string& primary = "unity.bin",
                 const std::string& mirror = "unity_mirror.bin");
    
    ~RAID1Manager();
    
    // Operaciones principales
    bool write(const std::string& data);
    std::string read();
    
    // Gestión del RAID
    bool synchronizeDisks();
    DiskStatus getPrimaryStatus() const { return primaryStatus; }
    DiskStatus getMirrorStatus() const { return mirrorStatus; }
    std::string getActiveDisk() const { return activeDisk; }
    
    // Recuperación
    bool rebuildMirror();
    bool rebuildPrimary();
};

#endif // RAID1MANAGER_H