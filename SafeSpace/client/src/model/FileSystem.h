#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include "iNode.hpp"
#include "Directory.hpp"

/**
 * @class FileSystem
 * @brief Main class for managing the file system, files, directory, and disk operations.
 */
class FileSystem {
private:
  std::string name; 
  uint64_t size;
  uint64_t free_size;
  uint64_t block_size;
  uint64_t blocks;
  std::string identified;
  std::string type;
  std::vector<bool> bitMap; 
  std::vector<iNode> files;
  uint64_t directionDirectory; 

  /**
   * @brief Resets the disk, filling it with zeros.
   */
  void resetUnity();

  /**
   * @brief Loads the bitmap from disk to memory.
   * @return 0 on success, -1 on error.
   */
  int loadBitMap();
  
public:
  FileSystem();
  ~FileSystem();

  Directory dir;

  /**
   * @brief Creates a new file in the file system.
   * @param filename Name of the file.
   * @param permissions File permissions (e.g., "rw").
   * @return 0 on success, -1 on error.
   */
  int createFile(const std::string& filename, std::string permissions);

  /**
   * @brief Opens a file for operations.
   * @param filename Name of the file.
   * @return 0 on success, -1 on error.
   */
  int openFile(const std::string filename);

  /**
   * @brief Closes an open file.
   * @param filename Name of the file.
   * @return 0 on success, -1 on error.
   */
  int closeFile(const std::string& filename);

  /**
   * @brief Writes data to a file.
   * @param filename Name of the file.
   * @param data Data to write.
   */
  void writeFile(std::string filename, std::string& data);

  /**
   * @brief Reads and prints the contents of a file.
   * @param filename Name of the file.
   */
  void readFile(const std::string filename);

  /**
   * @brief Deletes a file from the file system.
   * @param filename Name of the file.
   */
  void deleteFile(const std::string filename);

  /**
   * @brief Searches for a file in the directory and prints its iNode number.
   * @param filename Name of the file.
   */
  void searchFile(const std::string& filename);

  /**
   * @brief Saves an iNode to disk at the specified offset.
   * @param disk File stream for the disk.
   * @param node The iNode to save.
   * @param offset Offset in bytes where the iNode will be saved.
   */
  void saveInode(std::fstream& disk, const iNode& node, uint64_t offset);

  /**
   * @brief Initializes the directory structure on disk.
   * @return 0 on success, -1 on error.
   */
  int initializeDirectory();

  /**
   * @brief Saves the directory structure to disk.
   * @return 0 on success, -1 on error.
   */
  int saveDirectory();

  /**
   * @brief Loads the directory structure from disk.
   * @return 0 on success, -1 on error.
   */
  int loadDirectory();

  /**
   * @brief Loads an iNode from disk at the specified offset.
   * @param disk File stream for the disk.
   * @param offset Offset in bytes where the iNode is stored.
   * @return The loaded iNode.
   */
  iNode loadInode(std::fstream& disk, uint64_t offset);

  /**
   * @brief Renames a file in the directory and updates its iNode.
   * @param oldName Current name of the file.
   * @param newName New name for the file.
   * @return true if renamed successfully, false otherwise.
   */
  bool renameFile(const std::string& oldName, const std::string& newName);

  /**
   * @brief Lists all files in the directory.
   */
  void listFiles();

  /**
   * @brief Prints the status of the first 15 blocks in the bitmap.
   */
  void printBitMap();
};

