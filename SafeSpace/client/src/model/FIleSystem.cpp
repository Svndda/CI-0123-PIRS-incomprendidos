#include <fstream>
#include "FileSystem.h"
#include "File.hpp"
#include <iostream>

FileSystem::FileSystem(){
  this->name = "MyFileSystem";
  this->block_size = 256; // 256 bits = 32 bytes
  this->size = 1024 * 1024 * 1024; // 1 GB
  initializeDirectory();
  loadBitMap();
}

FileSystem::~FileSystem(){

}

int FileSystem::initializeDirectory(){
  std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
  if (!disk) {
    std::cerr << "Error initializing the disk" << std::endl;
    return -1;
  }

  disk.seekp(0, std::ios::beg);
  disk.read(reinterpret_cast<char*>(&this->directionDirectory), sizeof(this->directionDirectory));
  std::cout << "Directory initialized at block: " << this->directionDirectory << std::endl;
  disk.close();

  loadDirectory();
  
  std::cout << "Disk initialized successfully." << std::endl;
  return 0;
}


int FileSystem::loadBitMap() {
  std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
  if (!disk) {
    std::cerr << "Error initializing the disk" << std::endl;
    return -1;
  }
  disk.seekg(0, std::ios::end);
  this->blocks = this->size / this->block_size;
  this->bitMap.resize(this->blocks, false);
  for(size_t i = 0; i < this->blocks; ++i) {
    char byte;
    disk.seekg(i * this->block_size, std::ios::beg);
    disk.read(&byte, sizeof(byte));
    if(byte != 0) {
      this->bitMap[i] = true; // Bloque ocupado
    }
  }
  for(size_t i = 0; i < 15; ++i) {
    std::cout << "Block " << i << ": " << (this->bitMap[i] ? "Occupied" : "Free") << std::endl;
  }
  disk.close();
  
  return 0;
}
int FileSystem::createFile(const std::string& filename, std::string permissions) {
  if(this->dir.repeatName(filename)) {
    std::cerr << "File already exists." << std::endl;
    return -1;
  }

  uint64_t freeblock = 0;
  for(size_t i = 3; i < this->bitMap.size(); ++i) {
    if(!this->bitMap[i]) {
      this->bitMap[i] = true;
      freeblock = i;
      break;
    }
  }
  
  File newFile(freeblock, filename, permissions, freeblock);
  this->dir.addToDirectory(filename, newFile.getMetadata().id);

  std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
  if (!disk) {
    std::cerr << "Error initializing the disk" << std::endl;
    return -1;
  }
  saveInode(disk, newFile.getMetadata(), freeblock * this->block_size);
  saveDirectory();
  disk.close();
  dir.printDirectory();  
  return 0;
}


int FileSystem::openFile(const std::string filename) {
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {
    std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return -1;
    }
    iNode node = loadInode(disk, inodeNum * this->block_size);
    std::cout << "Opening file: " << filename << " with iNode: " << inodeNum << std::endl;
    node.state = "open";
    saveInode(disk, node, inodeNum * this->block_size);
    disk.close();
    return 0;
  } else {
    std::cout << "File not found." << std::endl;
    return -1;
  }
}

int FileSystem::closeFile(const std::string& filename) {
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {
    std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return -1;
    }
    iNode node = loadInode(disk, inodeNum * this->block_size);
    std::cout << "Opening file: " << filename << " with iNode: " << inodeNum << std::endl;
    node.state = "closed";
    saveInode(disk, node, inodeNum * this->block_size);
    disk.close();
    return 0;
  } else {
    std::cout << "File not found." << std::endl;
    return -1;
  }
}


void FileSystem::saveInode(std::fstream& disk, const iNode& node, uint64_t offset) {
  disk.seekp(offset, std::ios::beg);
  // Escribir y serializar la estructura iNode
  uint64_t nameLen = node.name.size();
  disk.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
  disk.write(node.name.c_str(), nameLen);

  
  uint64_t typeLen = node.type.size();
  disk.write(reinterpret_cast<const char*>(&typeLen), sizeof(typeLen));
  disk.write(node.type.c_str(), typeLen);

  
  uint64_t stateLen = node.state.size();
  disk.write(reinterpret_cast<const char*>(&stateLen), sizeof(stateLen));
  disk.write(node.state.c_str(), stateLen);

  
  disk.write(reinterpret_cast<const char*>(&node.id), sizeof(node.id));
  uint64_t permissionsLen = node.permissions.size();
  disk.write(reinterpret_cast<const char*>(&permissionsLen), sizeof(permissionsLen));
  disk.write(node.permissions.c_str(), permissionsLen);
  
  
  uint64_t blocksCount = node.blockPointers.size();
  disk.write(reinterpret_cast<const char*>(&blocksCount), sizeof(blocksCount));
  for (auto block : node.blockPointers) {
    disk.write(reinterpret_cast<const char*>(&block), sizeof(block));
  }
}

iNode FileSystem::loadInode(std::fstream& disk, uint64_t offset) {
    iNode node;
    disk.seekg(offset, std::ios::beg);

    // Leer y reconstruir la estructura iNode
    uint64_t nameLen;
    disk.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    node.name.resize(nameLen);
    disk.read(&node.name[0], nameLen);

    uint64_t typeLen;
    disk.read(reinterpret_cast<char*>(&typeLen), sizeof(typeLen));
    node.type.resize(typeLen);
    disk.read(&node.type[0], typeLen);


    uint64_t stateLen;
    disk.read(reinterpret_cast<char*>(&stateLen), sizeof(stateLen));
    node.state.resize(stateLen);
    disk.read(&node.state[0], stateLen);

    disk.read(reinterpret_cast<char*>(&node.id), sizeof(node.id));

    uint64_t permissionsLen;
    disk.read(reinterpret_cast<char*>(&permissionsLen), sizeof(permissionsLen));
    node.permissions.resize(permissionsLen);
    disk.read(&node.permissions[0], permissionsLen);

    uint64_t blocksCount;
    disk.read(reinterpret_cast<char*>(&blocksCount), sizeof(blocksCount));
    node.blockPointers.resize(blocksCount);
    for (uint64_t i = 0; i < blocksCount; ++i) {
        disk.read(reinterpret_cast<char*>(&node.blockPointers[i]), sizeof(uint64_t));
    }

    return node;
}

int FileSystem::saveDirectory() {
  std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
  if (!disk) {
    std::cerr << "Error initializing the disk" << std::endl;
  }

  disk.seekp(this->directionDirectory * this->block_size, std::ios::beg);
  uint64_t filesCount = this->dir.files.size();
  disk.write(reinterpret_cast<const char*>(&filesCount), sizeof(filesCount));
  for (const auto& entry : this->dir.files) {
    uint64_t nameLen = entry.filename.size();
    disk.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
    disk.write(entry.filename.c_str(), nameLen);
    disk.write(reinterpret_cast<const char*>(&entry.inodeNumber), sizeof(entry.inodeNumber));
  }
  std::cout << "Directory saved successfully." << std::endl;
  disk.close();
  return 0;
}

int FileSystem::loadDirectory() {
  std::fstream disk("./data/unity.bin", std::ios::in | std::ios::binary);
  if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return -1;
  }

  disk.seekg(this->directionDirectory * this->block_size, std::ios::beg);
  uint64_t filesCount = 0;
  disk.read(reinterpret_cast<char*>(&filesCount), sizeof(filesCount));
  this->dir.files.clear();

  for (uint64_t i = 0; i < filesCount; ++i) {
    Directory::Entry entry;
    uint64_t nameLen = 0;
    disk.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
    entry.filename.resize(nameLen);
    disk.read(&entry.filename[0], nameLen);
    disk.read(reinterpret_cast<char*>(&entry.inodeNumber), sizeof(entry.inodeNumber));
    this->dir.files.push_back(entry);
  }

  disk.close();
  std::cout << "Directory loaded successfully." << std::endl;
  return 0;
}

void FileSystem::resetUnity() {
  std::ofstream disk("./data/unity.bin", std::ios::out | std::ios::binary | std::ios::trunc);
  if (!disk) {
    std::cerr << "Error initializing the disk" << std::endl;
    return;
  }
  char zero = 0;
  for(size_t i = 0; i < this->blocks; ++i) {
    disk.write(&zero, sizeof(zero));
  }
  disk.close();
}

void FileSystem::writeFile(std::string filename, std::string& data) {
  
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {

    std::cout << "iNode number is: " << inodeNum << std::endl;

    std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return;
    }
    iNode node = loadInode(disk, inodeNum * this->block_size);
    if(node.state != "open") {
      std::cerr << "The file is not open. Open it before writing/deleting." << std::endl;
      disk.close();
      return;
    }

    size_t quantityBlocks = (data.size() + this->block_size - 1) / this->block_size;
    
    for(size_t i = 0; i < quantityBlocks; ++i) {
      for(size_t j = 3; j < this->bitMap.size(); ++j) {
        if(!this->bitMap[j]) {
          this->bitMap[j] = true;
          disk.seekp(j * this->block_size, std::ios::beg);
          disk.write(data.c_str() + i * this->block_size, std::min(this->block_size, data.size() - i * this->block_size));
          node.blockPointers.push_back(j);
          break;
        }
      }
    }
    saveInode(disk, node, inodeNum * this->block_size);
    disk.close();

  } else {
    std::cout << "File not found." << std::endl;
  }

}

void FileSystem::readFile(std::string filename) {
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {
    std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return;
    }
  iNode node = loadInode(disk, inodeNum * this->block_size);
  if(node.state != "open") {
    std::cerr << "The file is not open. Open it before reading." << std::endl;
    disk.close();
    return;
  }
    std::cout << "iNode number is: " << inodeNum << std::endl;
    std::string data;
    for(auto block : node.blockPointers) {
      char buffer[256] = {0};
      disk.seekg(block * this->block_size, std::ios::beg);
      disk.read(buffer, this->block_size);
      data.append(buffer, this->block_size);
    }
    std::cout << "Data read from file: " << data << std::endl;
    disk.close();
  } else {
    std::cout << "File not found." << std::endl;
  }  
}

void FileSystem::deleteFile(const std::string filename) {
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {
    std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
    if (!disk) {
      std::cerr << "Error initializing the disk" << std::endl;
      return;
    }
    iNode node = loadInode(disk, inodeNum * this->block_size);
    for(auto block : node.blockPointers) {
      char zero = 0;
      disk.seekp(block * this->block_size, std::ios::beg);
      disk.write(&zero, sizeof(zero));
      this->bitMap[block] = false;
    }
    char zero = 0;
    disk.seekp(inodeNum * this->block_size, std::ios::beg);
    disk.write(&zero, sizeof(zero));
    this->bitMap[inodeNum] = false;
    this->dir.removeFromDirectory(filename);
    saveDirectory();

    std::cout << "File deleted successfully." << std::endl;
    disk.close();
  } else {
    std::cout << "File not found." << std::endl;
  }
}
  
void FileSystem::listFiles() {
    dir.listFiles();
}

void FileSystem::searchFile(const std::string& filename) {
  uint64_t inodeNum = this->dir.findInDirectory(filename);
  if (inodeNum != UINT64_MAX) {
    std::cout << "File found: " << filename << " with iNode: " << inodeNum << std::endl;
  } else {
    std::cout << "File not found." << std::endl;
  }
}
bool FileSystem::renameFile(const std::string& oldName, const std::string& newName) {
  if(this->dir.repeatName(newName)) {
    std::cerr << "The new name already exists." << std::endl;
    return false;
  }
  bool result = this->dir.renameFile(oldName, newName);
  if(result) {
    saveDirectory();
    uint64_t inodeNum = this->dir.findInDirectory(oldName);
    if (inodeNum != UINT64_MAX) {
      std::fstream disk("./data/unity.bin", std::ios::in | std::ios::out | std::ios::binary);
      if (!disk) {
        std::cerr << "Error initializing the disk" << std::endl;
        return false;
      }
      iNode node = loadInode(disk, inodeNum * this->block_size);
      node.name = newName;
      saveInode(disk, node, inodeNum * this->block_size);
      disk.close();
    }
    std::cout << "File renamed successfully from '" << oldName << "' to '" << newName << "'." << std::endl;
  } else {
    std::cerr << "Failed to rename the file. Please check if the file exists." << std::endl;
  }
  return result;
}

void FileSystem::printBitMap() {
  for(size_t i = 0; i < 15; ++i) {
    std::cout << "Block " << i << ": " << (this->bitMap[i] ? "Occupied" : "Free") << std::endl;
  }
}