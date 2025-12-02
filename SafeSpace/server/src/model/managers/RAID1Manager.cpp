#include "RAID1Manager.h"
#include "FileSystem.h"
#include <iostream>

RAID1Manager::RAID1Manager(FileSystem& fs, 
                           const std::string& primary,
                           const std::string& mirror)
    : fileSystem(fs),
      primaryDisk(primary),
      mirrorDisk(mirror),
      primaryStatus(DiskStatus::OPERATIONAL),
      mirrorStatus(DiskStatus::OPERATIONAL),
      activeDisk(primary) {
    
    // Abrir ambos discos
    try {
        fileSystem.openFile(primaryDisk);
        std::cout << "Disco primario '" << primaryDisk << "' abierto." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error al abrir disco primario: " << e.what() << std::endl;
        primaryStatus = DiskStatus::FAILED;
    }
    
    try {
        fileSystem.openFile(mirrorDisk);
        std::cout << "Disco espejo '" << mirrorDisk << "' abierto." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error al abrir disco espejo: " << e.what() << std::endl;
        mirrorStatus = DiskStatus::FAILED;
    }
    
    // Si el primario está caído, usar el espejo
    if (primaryStatus == DiskStatus::FAILED && mirrorStatus == DiskStatus::OPERATIONAL) {
        switchToMirror();
    }
    
    // Sincronizar discos si ambos están operativos
    if (primaryStatus == DiskStatus::OPERATIONAL && 
        mirrorStatus == DiskStatus::OPERATIONAL) {
        synchronizeDisks();
    }
}

RAID1Manager::~RAID1Manager() {
    try {
        if (primaryStatus != DiskStatus::FAILED) {
            fileSystem.closeFile(primaryDisk);
        }
        if (mirrorStatus != DiskStatus::FAILED) {
            fileSystem.closeFile(mirrorDisk);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error al cerrar discos: " << e.what() << std::endl;
    }
}

bool RAID1Manager::write(const std::string& data) {
    std::lock_guard<std::mutex> lock(diskMutex);
    
    bool primaryWritten = false;
    bool mirrorWritten = false;
    
    // Paso 1: Escribir en el disco primario primero
    if (primaryStatus == DiskStatus::OPERATIONAL) {
        primaryWritten = writeToDisk(primaryDisk, data);
        if (!primaryWritten) {
            std::cerr << "Fallo al escribir en disco primario." << std::endl;
            primaryStatus = DiskStatus::FAILED;
            // Cambiar a espejo si está disponible
            if (mirrorStatus == DiskStatus::OPERATIONAL) {
                switchToMirror();
            }
        }
    }
    
    // Paso 2: Solo después de escribir en primario, escribir en espejo
    if (mirrorStatus == DiskStatus::OPERATIONAL) {
        // Control de concurrencia: solo escribir si el primario lo hizo
        // o si el primario está caído
        if (primaryWritten || primaryStatus == DiskStatus::FAILED) {
            mirrorWritten = writeToDisk(mirrorDisk, data);
            if (!mirrorWritten) {
                std::cerr << "Fallo al escribir en disco espejo." << std::endl;
                mirrorStatus = DiskStatus::FAILED;
            }
        }
    }
    
    // Retornar true si al menos uno de los discos escribió correctamente
    bool success = primaryWritten || mirrorWritten;
    
    if (!success) {
        std::cerr << "RAID 1 CRÍTICO: Ambos discos fallaron en escritura." << std::endl;
    } else if (primaryStatus == DiskStatus::FAILED || mirrorStatus == DiskStatus::FAILED) {
        std::cerr << "RAID 1 en modo DEGRADADO: Un disco ha fallado." << std::endl;
    }
    
    return success;
}

std::string RAID1Manager::read() {
    std::lock_guard<std::mutex> lock(diskMutex);
    
    // Leer del disco activo
    std::string data;
    
    try {
        data = readFromDisk(activeDisk);
    } catch (const std::exception& e) {
        std::cerr << "Error al leer de " << activeDisk << ": " << e.what() << std::endl;
        
        // Si falla, intentar con el otro disco
        if (activeDisk == primaryDisk && mirrorStatus == DiskStatus::OPERATIONAL) {
            std::cout << "Cambiando a disco espejo para lectura..." << std::endl;
            switchToMirror();
            try {
                data = readFromDisk(activeDisk);
            } catch (const std::exception& e2) {
                std::cerr << "Error crítico: ambos discos inaccesibles." << std::endl;
                throw;
            }
        } else if (activeDisk == mirrorDisk && primaryStatus == DiskStatus::OPERATIONAL) {
            std::cout << "Cambiando a disco primario para lectura..." << std::endl;
            switchToPrimary();
            try {
                data = readFromDisk(activeDisk);
            } catch (const std::exception& e2) {
                std::cerr << "Error crítico: ambos discos inaccesibles." << std::endl;
                throw;
            }
        }
    }
    
    return data;
}

bool RAID1Manager::writeToDisk(const std::string& diskName, const std::string& data) {
    try {
        fileSystem.write(diskName, data);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error escribiendo a " << diskName << ": " << e.what() << std::endl;
        return false;
    }
}

std::string RAID1Manager::readFromDisk(const std::string& diskName) {
    try {
        return fileSystem.read(diskName);
    } catch (const std::exception& e) {
        std::cerr << "Error leyendo de " << diskName << ": " << e.what() << std::endl;
        throw;
    }
}

void RAID1Manager::switchToMirror() {
    activeDisk = mirrorDisk;
    primaryStatus = DiskStatus::FAILED;
    std::cout << "RAID 1: Cambiado a disco espejo como activo." << std::endl;
}

void RAID1Manager::switchToPrimary() {
    activeDisk = primaryDisk;
    mirrorStatus = DiskStatus::FAILED;
    std::cout << "RAID 1: Cambiado a disco primario como activo." << std::endl;
}

bool RAID1Manager::synchronizeDisks() {
    std::lock_guard<std::mutex> lock(diskMutex);
    
    if (primaryStatus != DiskStatus::OPERATIONAL || 
        mirrorStatus != DiskStatus::OPERATIONAL) {
        std::cerr << "No se puede sincronizar: uno o ambos discos no están operativos." << std::endl;
        return false;
    }
    
    try {
        // Copiar datos del primario al espejo
        std::string primaryData = readFromDisk(primaryDisk);
        writeToDisk(mirrorDisk, primaryData);
        std::cout << "Discos sincronizados correctamente." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error durante sincronización: " << e.what() << std::endl;
        return false;
    }
}

bool RAID1Manager::rebuildMirror() {
    if (primaryStatus != DiskStatus::OPERATIONAL) {
        std::cerr << "No se puede reconstruir espejo: disco primario no operativo." << std::endl;
        return false;
    }
    
    std::cout << "Reconstruyendo disco espejo desde primario..." << std::endl;
    mirrorStatus = DiskStatus::OPERATIONAL;
    return synchronizeDisks();
}

bool RAID1Manager::rebuildPrimary() {
    if (mirrorStatus != DiskStatus::OPERATIONAL) {
        std::cerr << "No se puede reconstruir primario: disco espejo no operativo." << std::endl;
        return false;
    }
    
    std::lock_guard<std::mutex> lock(diskMutex);
    
    try {
        std::cout << "Reconstruyendo disco primario desde espejo..." << std::endl;
        std::string mirrorData = readFromDisk(mirrorDisk);
        writeToDisk(primaryDisk, mirrorData);
        primaryStatus = DiskStatus::OPERATIONAL;
        activeDisk = primaryDisk; // Volver a usar el primario
        std::cout << "Disco primario reconstruido correctamente." << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error reconstruyendo primario: " << e.what() << std::endl;
        return false;
    }
}