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

int FileSystem::create(const std::string& name) {
    if (name.empty()) return -1;
    if (dirFind(name) >= 0) {
        std::cerr << "[FS] Ya existe: " << name << "\n";
        return -1;
    }

    int inodeId = allocateInode();
    if (inodeId < 0) {
        std::cerr << "[FS] No hay i-nodos libres.\n";
        return -1;
    }

    iNode n{};
    n.inode_id    = static_cast<uint64_t>(inodeId);
    n.size_bytes  = 0;
    n.blocks_used = 0;
    std::memset(n.direct, 0, sizeof(n.direct));
    n.indirect1   = 0;
    n.flags       = 1; // activo

    if (!disk.writeInode(inodeOffset(inodeId), n)) {
        std::cerr << "[FS] Error al persistir i-nodo.\n";
        return -1;
    }
    inodeTable[inodeId] = n;

    if (!dirAdd(name, static_cast<uint64_t>(inodeId))) {
        std::cerr << "[FS] Directorio lleno o error al guardar.\n";
        freeInode(inodeId);
        return -1;
    }

    return inodeId;
}

int FileSystem::find(const std::string& name) const {
    int didx = dirFind(name);
    if (didx < 0) return -1;
    return static_cast<int>(directory[didx].inode_id);
}

int FileSystem::dirFindByInode(uint64_t inodeId) const {
    for (size_t i = 0; i < directory.size(); ++i) {
        if (directory[i].inode_id == inodeId) return static_cast<int>(i);
    }
    return -1;
}
bool FileSystem::write(const std::string& name, const std::string& data) {
    int inodeId = find(name);
    if (inodeId < 0) {
        std::cerr << "[FS] No existe: " << name << "\n";
        return false;
    }

    iNode& n = inodeTable[inodeId];

    // Escribir a bloques directos
    size_t remaining = data.size();
    size_t cursor = 0;

    for (int i = 0; i < 10 && remaining > 0; ++i) {
        if (n.direct[i] == 0) {
            int b = allocateBlock();
            if (b < 0) { std::cerr << "[FS] Sin bloques libres.\n"; return false; }
            n.direct[i] = static_cast<uint32_t>(b);
        }
        size_t chunk = std::min(remaining, static_cast<size_t>(superBlock.block_size));
        if (!disk.writeBytes(dataBlockOffset(n.direct[i]), data.data() + cursor, chunk))
            return false;

        // Si el chunk es menor al bloque, limpia el resto (opcional)
        if (chunk < superBlock.block_size) {
            std::vector<uint8_t> zeros(superBlock.block_size - chunk, 0);
            if (!disk.writeBytes(dataBlockOffset(n.direct[i]) + chunk, zeros.data(), zeros.size()))
                return false;
        }

        cursor += chunk;
        remaining -= chunk;
    }

    // Si faltan datos, usar indirecto simple
    if (remaining > 0) {
        if (n.indirect1 == 0) {
            int ib = allocateBlock(); // bloque de índices
            if (ib < 0) { std::cerr << "[FS] Sin bloques para índice.\n"; return false; }
            n.indirect1 = static_cast<uint32_t>(ib);
            // inicializa bloque de índices a cero
            std::vector<uint8_t> zeros(superBlock.block_size, 0);
            disk.writeBytes(dataBlockOffset(n.indirect1), zeros.data(), zeros.size());
        }

        // cargar tabla de índices
        const size_t idxCount = superBlock.block_size / sizeof(uint32_t);
        std::vector<uint32_t> idx(idxCount, 0);
        disk.readBytes(dataBlockOffset(n.indirect1), idx.data(), superBlock.block_size);

        for (size_t k = 0; k < idxCount && remaining > 0; ++k) {
            if (idx[k] == 0) {
                int b = allocateBlock();
                if (b < 0) { std::cerr << "[FS] Sin bloques libres (indirecto).\n"; return false; }
                idx[k] = static_cast<uint32_t>(b);
            }

            size_t chunk = std::min(remaining, static_cast<size_t>(superBlock.block_size));
            if (!disk.writeBytes(dataBlockOffset(idx[k]), data.data() + cursor, chunk))
                return false;

            if (chunk < superBlock.block_size) {
                std::vector<uint8_t> zeros(superBlock.block_size - chunk, 0);
                if (!disk.writeBytes(dataBlockOffset(idx[k]) + chunk, zeros.data(), zeros.size()))
                    return false;
            }

            cursor += chunk;
            remaining -= chunk;
        }

        // guardar tabla de índices
        disk.writeBytes(dataBlockOffset(n.indirect1), idx.data(), superBlock.block_size);
    }

    // actualizar metadatos
    // (no usamos ctime/mtime/atime por ahora)
    n.size_bytes  = data.size();
    n.blocks_used = 0;
    for (int i = 0; i < 10; ++i) if (n.direct[i]) n.blocks_used++;
    if (n.indirect1) {
        // contar bloques apuntados por indirecto
        const size_t idxCount = superBlock.block_size / sizeof(uint32_t);
        std::vector<uint32_t> idx(idxCount, 0);
        disk.readBytes(dataBlockOffset(n.indirect1), idx.data(), superBlock.block_size);
        for (auto v : idx) if (v) n.blocks_used++;
        n.blocks_used++; // cuenta también el bloque de índices si querés
    }

    if (!disk.writeInode(inodeOffset(inodeId), n)) return false;
    if (!disk.saveBitMap(bitMap, superBlock)) return false;
    return true;
}

std::string FileSystem::read(const std::string& name) {
    int inodeId = find(name);
    if (inodeId < 0) return {};

    const iNode& n = inodeTable[inodeId];
    if (n.size_bytes == 0) return {};

    std::string out;
    out.reserve(n.size_bytes);

    // directos
    std::vector<char> buf(superBlock.block_size);
    size_t remaining = n.size_bytes;

    for (int i = 0; i < 10 && remaining > 0; ++i) {
        if (n.direct[i] == 0) break;
        disk.readBytes(dataBlockOffset(n.direct[i]), buf.data(), superBlock.block_size);
        size_t chunk = std::min(remaining, static_cast<size_t>(superBlock.block_size));
        out.append(buf.data(), chunk);
        remaining -= chunk;
    }

    // indirectos
    if (remaining > 0 && n.indirect1 != 0) {
        const size_t idxCount = superBlock.block_size / sizeof(uint32_t);
        std::vector<uint32_t> idx(idxCount, 0);
        disk.readBytes(dataBlockOffset(n.indirect1), idx.data(), superBlock.block_size);

        for (size_t k = 0; k < idxCount && remaining > 0; ++k) {
            if (idx[k] == 0) break;
            disk.readBytes(dataBlockOffset(idx[k]), buf.data(), superBlock.block_size);
            size_t chunk = std::min(remaining, static_cast<size_t>(superBlock.block_size));
            out.append(buf.data(), chunk);
            remaining -= chunk;
        }
    }

    return out;
}

bool FileSystem::remove(const std::string& name) {
    int inodeId = find(name);
    if (inodeId < 0) return false;

    iNode& n = inodeTable[inodeId];

    // liberar directos
    for (int i = 0; i < 10; ++i) {
        if (n.direct[i]) {
            freeBlock(n.direct[i]);
            n.direct[i] = 0;
        }
    }

    // liberar indirecto
    if (n.indirect1) {
        const size_t idxCount = superBlock.block_size / sizeof(uint32_t);
        std::vector<uint32_t> idx(idxCount, 0);
        disk.readBytes(dataBlockOffset(n.indirect1), idx.data(), superBlock.block_size);
        for (auto v : idx) if (v) freeBlock(v);
        freeBlock(n.indirect1);
        n.indirect1 = 0;
    }

    // borrar del directorio
    int dIdx = dirFindByInode(inodeId);
    if (dIdx >= 0) dirRemoveByIndex(dIdx);

    // liberar i-nodo
    freeInode(inodeId);

    // persistir cambios
    disk.saveBitMap(bitMap, superBlock);
    disk.writeInode(inodeOffset(inodeId), n);

    std::cout << "[FS] Eliminado: " << name << "\n";
    return true;
}


