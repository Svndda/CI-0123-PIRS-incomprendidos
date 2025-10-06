#pragma once
#include "DiskManager.h"
#include "iNode.h"
#include "Layout.h"
#include "DirEntry.h"
#include <string>
#include <vector>

class FileSystem {
private:
    DiskManager disk;
    std::vector<bool> bitMap;           // mapa de bloques
    std::vector<iNode> inodeTable;      // tabla de i-nodos
    std::vector<DirEntry> directory;    // entradas de directorio
    Layout::superBlock superBlock;

    void computeSuperAndOffsets();     // rellena superBlock con Layout::registerOffsets
    bool writeSuperToDisk();           // escribe superbloque
    bool readSuperFromDisk();          // lee superbloque desde disco
    // Funciones auxiliares
    int allocateBlock();               // busca un bloque libre
    void freeBlock(uint32_t blockID);  // libera un bloque
    int allocateInode();               // busca un inode libre
    void freeInode(uint32_t inodeID);  // libera un inode
    uint64_t inodeOffset(uint64_t inodeId);



    // directorio
    int  dirFind(const std::string& name) const;     // idx en directory o -1
    int  dirFindByInode(uint64_t inodeId) const;     // idx en directory o -1
    bool dirAdd(const std::string& name, uint64_t inodeId);
    bool dirRemoveByIndex(int idx);
    bool loadDirectoryFromDisk();
    bool saveDirectoryToDisk();

    // datos
    uint64_t dataBlockOffset(uint32_t blockId) const {
        return superBlock.data_area_offset + static_cast<uint64_t>(blockId) * superBlock.block_size;
    }

public:
    FileSystem(const std::string& diskPath);

    // Operaciones principales
    bool format();                     // formatea el disco (superblock + bitmap + inodes vacíos)
    bool mount();                      // carga estructuras desde el disco
    int  create(const std::string& name); // crea un nuevo archivo
    bool write(const std::string& name, const std::string& data);
    std::string read(const std::string& name);
    bool remove(const std::string& name);
    int  find(const std::string& name) const; // retorna el id del i-nodo

    // Debug
    void listFiles() const;

    // Estado
    bool isValid() const;
};
