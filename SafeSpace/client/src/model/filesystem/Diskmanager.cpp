#include "Diskmanager.h"


DiskManager::DiskManager(const std::string& path){

}
DiskManager::~DiskManager(){

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

    disk.seekp(offset, std::ios::beg);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al posicionar el puntero de escritura.\n";
        return false;
    }

    disk.read(reinterpret_cast<char*>(buffer), bytes);
    if (!disk.good()) {
        std::cerr << "[DiskManager] Error: fallo al leer en el disco.\n";
        return false;
    }

    return true;
}


void DiskManager::resetUnity(){
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


int DiskManager::loadBitMap(){
    if (!disk.is_open()) {
        std::cerr << "[DiskManager] Error: disco no abierto para leer bitmap.\n";
        return -1;
    }

    constexpr uint64_t BITMAP_OFFSET = 256;                          // ejemplo, después del superbloque
    constexpr uint64_t BITMAP_BYTES = Layout::BLOCK_COUNT / 8;       // ejemplo: 4 millones de bloques → 512 KB

    std::vector<char> buffer(BITMAP_BYTES);

    if (!readBytes(BITMAP_OFFSET, buffer.data(), BITMAP_BYTES))
        return -1;

    outBitmap.resize(Layout::BLOCK_COUNT);
    for (size_t i = 0; i < Layout::BLOCK_COUNT; ++i) {
        size_t byteIndex = i / 8;
        size_t bitIndex  = i % 8;
        outBitmap[i] = (buffer[byteIndex] >> bitIndex) & 1;
    }

    return 0;
}


bool DiskManager::writeInode(uint64_t offset, const iNode& inode){
    return writeBytes(offset, &inode, sizeof(&inode));
}

bool DiskManager::readInode(uint64_t offset, iNode& outInode){
    return readBytes(offset, &outInode, sizeof(iNode));
}
