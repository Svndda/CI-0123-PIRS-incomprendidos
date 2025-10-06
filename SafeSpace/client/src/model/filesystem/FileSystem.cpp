#include "FileSystem.h"

FileSystem::FileSystem() {}

static void copyNameFixed(char dst[Layout::DIR_NAME_LEN], const std::string& src) {
    std::memset(dst, 0, Layout::DIR_NAME_LEN);
    std::strncpy(dst, src.c_str(), Layout::DIR_NAME_LEN - 1);
}

void FileSystem::computeSuperAndOffsets() {
    superBlock = {}; // por si acaso
    Layout::registerOffsets(superBlock);
}

bool FileSystem::writeSuperToDisk() {
    return disk.writeBytes(superBlock.super_offset, &superBlock, sizeof(superBlock));
}

bool FileSystem::readSuperFromDisk() {
    return disk.readBytes(0, &superBlock, sizeof(superBlock));
}



bool FileSystem::format() {
    std::cout << "[FS] Formateando disco...\n";
    disk.resetUnity();

    bitMap.assign(superBlock.block_count, false);
    inodeTable.assign(superBlock.inode_count, {});

    // Escribir bitmap en disco
    disk.saveBitMap(bitMap, superBlock);

    // Escribir inodos vacíos
    for (uint64_t i = 0; i < superBlock.inode_count; ++i) {
        iNode empty{};
        disk.writeInode(superBlock.inode_table_offset + i * superBlock.inode_size, empty);
    }

    // Escribir superbloque al disco
    if (!writeSuperToDisk()) {
        std::cerr << "[FS] Error: no se pudo escribir el superbloque.\n";
        return false;
    }

    directory.clear();
    std::cout << "[FS] Formato completado.\n";
    return true;
}

bool FileSystem::mount() {
    std::cout << "[FS] Montando...\n";

    // Intentar leer superbloque. Si está vacío, calculamos offsets por Layout.
    if (!readSuperFromDisk()) {
        computeSuperAndOffsets();
    } else {
        // (opcional) validar tamaños esperados; si no, recomputar
        if (superBlock.block_size != Layout::BLOCK_SIZE ||
            superBlock.inode_size != Layout::INODE_SIZE) {
            computeSuperAndOffsets();
        }
    }

    // Cargar bitmap
    if (disk.loadBitMap(bitMap, superBlock) != 0) {
        // Si no existe aún, inicializa un bitmap vacío (posible primer uso)
        bitMap.assign(superBlock.block_count, false);
    }

    // Cargar i-nodos a memoria
    inodeTable.resize(superBlock.inode_count);
    for (uint64_t i = 0; i < superBlock.inode_count; ++i) {
        disk.readInode(superBlock.inode_table_offset + i * superBlock.inode_size, inodeTable[i]);
    }

    // Cargar directorio
    if (!loadDirectoryFromDisk()) {
        directory.assign(superBlock.dir_entry_count, DirEntry{});
    }

    std::cout << "[FS] Montado.\n";
    return true;
}

int FileSystem::dirFind(const std::string& name) const {
    for (size_t i = 0; i < directory.size(); ++i) {
        if (directory[i].inode_id != 0 && std::strncmp(directory[i].name, name.c_str(), Layout::DIR_NAME_LEN) == 0)
            return static_cast<int>(i);
    }
    return -1;
}

bool FileSystem::dirAdd(const std::string& name, uint64_t inodeId) {
    if (name.empty()) return false;
    if (dirFind(name) >= 0) return false; // ya existe

    for (auto& e : directory) {
        if (e.inode_id == 0) {
            copyNameFixed(e.name, name);
            e.inode_id = inodeId;
            return saveDirectoryToDisk();
        }
    }
    return false; // sin espacio en directorio raíz
}

bool FileSystem::dirRemoveByIndex(int idx) {

    if (idx < 0 || static_cast<size_t>(idx) >= directory.size()) return false;
    directory[idx] = DirEntry{}; // borrar
    return saveDirectoryToDisk();

}

bool FileSystem::loadDirectoryFromDisk() {
    const uint64_t bytes = static_cast<uint64_t>(superBlock.dir_entry_count) * superBlock.dir_entry_size;
    std::vector<uint8_t> buf(bytes, 0);
    if (!disk.readBytes(superBlock.directory_offset, buf.data(), buf.size())){
        return false;
    }

    // desempaquetar
    directory.assign(superBlock.dir_entry_count, DirEntry{});
    const size_t entrySize = sizeof(DirEntry);
    for (size_t i = 0; i < superBlock.dir_entry_count; ++i) {
        std::memcpy(&directory[i], buf.data() + i * entrySize, entrySize);
    }
    return true;
}

bool FileSystem::saveDirectoryToDisk() {
    const size_t entrySize = sizeof(DirEntry);
    std::vector<uint8_t> buf(directory.size() * entrySize, 0);
    for (size_t i = 0; i < directory.size(); ++i) {
        std::memcpy(buf.data() + i * entrySize, &directory[i], entrySize);
    }
    return disk.writeBytes(superBlock.directory_offset, buf.data(), buf.size());
}


