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
