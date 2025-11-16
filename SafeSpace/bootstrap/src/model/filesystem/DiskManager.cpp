#include "DiskManager.h"
#include <cstring>

//DiskManager::DiskManager() {}

DiskManager::DiskManager(const std::string& path) : diskPath(path) {}

DiskManager::~DiskManager() {
    closeDisk();
}

bool DiskManager::openDisk(std::ios::openmode mode){
    // Cierra cualquier archivo anterior
    if (this->disk.is_open())
        this->disk.close();

    // Abre el archivo binario con el modo indicado
    this->disk.open(diskPath, mode);

    // Verifica si se abrió correctamente
    if (!this->disk.is_open()) {
        std::cerr << "[DiskManager] Error: No se pudo abrir el disco en la ruta: "
                  << diskPath << std::endl;
        return false;
    }

    // Coloca punteros al inicio por seguridad
    this->disk.seekg(0, std::ios::beg);
    this->disk.seekp(0, std::ios::beg);

    return true;
}

void DiskManager::closeDisk(){
    if (this->disk.is_open()) {
        this->disk.close();
    }
}

bool DiskManager::isOpen() const {
    return disk.is_open();
}

bool DiskManager::writeBytes(uint64_t offset, const void* buffer, size_t bytes){
    if (!disk.is_open()) {
        std::cerr << "[DiskManager] Error: el disco no está abierto para escritura.\n";
        return false;
    }

    disk.seekp(offset, std::ios::beg);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al posicionar el puntero de escritura.\n";
        return false;
    }

    disk.write(reinterpret_cast<const char*>(buffer), bytes);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al escribir en el disco.\n";
        return false;
    }

    disk.flush(); // fuerza la escritura al archivo
    return true;
}

bool DiskManager::readBytes(uint64_t offset, void* buffer, size_t bytes){
    if (!disk.is_open()) {
        std::cerr << "[DiskManager] Error: el disco no está abierto para escritura.\n";
        return false;
    }

    disk.seekg(offset, std::ios::beg);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al posicionar el puntero de lectura.\n";
        return false;
    }

    disk.read(reinterpret_cast<char*>(buffer), bytes);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al leer en el disco.\n";
        return false;
    }

    return true;
}


void DiskManager::resetUnity() {
    std::ofstream newDisk(diskPath, std::ios::binary | std::ios::trunc);
    if (!newDisk.is_open()) {
        std::cerr << "[DiskManager] Error: no se pudo crear el disco: " << diskPath << "\n";
        return;
    }

    // 1 GB = 1'073'741'824 bytes
    constexpr uint64_t DISK_SIZE = 1024ull * 1024ull * 1024ull;
    constexpr size_t BUFFER_SIZE = 4096;
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, BUFFER_SIZE);

    uint64_t written = 0;
    while (written < DISK_SIZE) {
        newDisk.write(buffer, BUFFER_SIZE);
        written += BUFFER_SIZE;
    }

    newDisk.close();
    std::cout << "[DiskManager] Disco reiniciado con éxito (" << DISK_SIZE / (1024*1024)
              << " MB llenos de ceros)\n";
}


bool DiskManager::saveBitMap(const std::vector<bool>& bitMap, const Layout::superBlock& superBlock) {
    if (!disk.is_open()) {
        std::cerr << "[DiskManager] Error: disco no abierto para escribir bitmap.\n";
        return false;
    }

    const uint64_t offset     = superBlock.bitmap_offset;
    const uint64_t bytesCount = Layout::bitmapBytes(superBlock.block_count);

    std::vector<uint8_t> buffer(bytesCount, 0);

    for (uint64_t i = 0; i < bitMap.size(); ++i) {
        if (bitMap[i]) {
            const uint64_t byteIndex = i / 8;
            const uint8_t  bitIndex  = static_cast<uint8_t>(i % 8);
            if (byteIndex < buffer.size()) {
                buffer[byteIndex] |= (1u << bitIndex);
            }
        }
    }

    return writeBytes(offset, buffer.data(), buffer.size());
}

int DiskManager::loadBitMap(std::vector<bool>& outBitmap, const Layout::superBlock& superBlock){
    if (!disk.is_open()) {
        std::cerr << "[DiskManager] Error: disco no abierto para leer bitmap.\n";
        return -1;
    }

    const uint64_t bitmapOffset = superBlock.bitmap_offset;
    const uint64_t bitmapBytes = Layout::bitmapBytes(superBlock.block_count);

    std::vector<char> buffer(bitmapBytes);

    if (!readBytes(bitmapOffset, buffer.data(), bitmapBytes))
        return -1;

    outBitmap.resize(superBlock.block_count);
    for (size_t i = 0; i < superBlock.block_count; ++i) {
        size_t byteIndex = i / 8;
        size_t bitIndex  = i % 8;
        outBitmap[i] = (buffer[byteIndex] >> bitIndex) & 1;
    }

    return 0;
}


bool DiskManager::writeInode(uint64_t offset, const iNode& inode){
    return writeBytes(offset, &inode, sizeof(inode));
}

bool DiskManager::readInode(uint64_t offset, iNode& outInode){
    return readBytes(offset, &outInode, sizeof(iNode));
}
