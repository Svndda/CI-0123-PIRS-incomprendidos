#include "File.hpp"
#include <stdexcept>
#include <iostream>
#include <ctime>

File::File(
  uint64_t id, const std::string &fileName,
  const std::string &permissions, uint64_t block) {

  this->metadata.id = id;
  this->metadata.name = fileName;
  this->metadata.type = ".txt";
  this->metadata.state = "closed";
  this->metadata.length = 0;
  this->metadata.fileSize = 0;
  this->metadata.creationTime = std::time(nullptr);
  this->metadata.modificationTime = std::time(nullptr);
  this->metadata.blockPointers.emplace_back(block);
  this->metadata.permissions = permissions; // default permissions
}

File::File() {
  this->metadata.id = 0;
  this->metadata.name = "newfile";
  this->metadata.type = ".txt";
  this->metadata.state = "closed";
  this->metadata.length = 0;
  this->metadata.fileSize = 0;
  this->metadata.creationTime = std::time(nullptr);
  this->metadata.modificationTime = std::time(nullptr);
  this->metadata.blockPointers.clear();
  this->metadata.permissions = "rw-r--r--"; // default permissions
}

// Destructor closes file if still open to release resources
File::~File() {
  if (this->metadata.state == "open") {
    this->close();
  }
}

// Opens the file if permissions allow
bool File::open() {
  if (this->metadata.state == "open") {
    std::cerr << "File already open.\n";
    return false;
  }
  // Simple permission check for read or write (simulated)
  if (this->metadata.permissions.find('r') == std::string::npos &&
      this->metadata.permissions.find('w') == std::string::npos) {
    throw std::runtime_error("Permission denied to open the file.");
  }
  this->metadata.state = "open";
  this->metadata.modificationTime = std::time(nullptr);
  std::cout << "File opened successfully.\n";
  return true;
}

// Closes the file if open
bool File::close() {
  if (this->metadata.state != "open") {
    std::cerr << "File is not open; cannot close.\n";
    return false;
  }
  this->metadata.state = "closed";
  this->metadata.modificationTime = std::time(nullptr);
  std::cout << "File closed successfully.\n";
  return true;
}

bool File::truncate() {
  if (this->metadata.state == "open") {
    std::cerr << "File is open, cannot truncate it.\n";
    return false;
  }

  this->metadata.modificationTime = std::time(nullptr);
  this->metadata.fileSize = 0;
  this->metadata.length = 0;
  return true;
}

bool File::deleteFile() {
  if (this->metadata.state == "open") {
    std::cerr << "File is open, cannot delete it.\n";
    return false;
  }
  this->metadata.id = 0;
  this->metadata.name.clear();
  this->metadata.type.clear();
  this->metadata.state.clear();
  this->metadata.length = 0;
  this->metadata.fileSize = 0;
  this->metadata.creationTime = 0;
  this->metadata.modificationTime = 0;
  this->metadata.blockPointers.clear();

  std::cout << "File deleted successfully.\n";
  return true;
}
