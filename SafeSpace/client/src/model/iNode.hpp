#pragma once

#include <iostream>
#include <ctime>
#include <string>
#include <vector>
#include <cstdint>

/**
 * @struct iNode
 * @brief Represents the metadata of a file.
 *
 * The iNode contains attributes describing a file, including identification,
 * size, timestamps, permissions, and block pointers used for data storage.
 */
struct iNode {
  /** File unique identifier (inode number). */
  uint64_t id;
  /** File name. */
  std::string name;
  /** File extension type. */
  std::string type;
  /** File state (e.g. open, closed)*/
  std::string state;
  /** File length in bytes*/
  uint64_t length;
  /** File size in blocks.*/
  uint64_t fileSize;
  /** File creation timestamp*/
  std::time_t creationTime;
  /** File last modification timestamp.*/
  std::time_t modificationTime;
  /** File related block pointers.*/
  std::vector<uint64_t> blockPointers;
  /** File declared permissions.*/
  std::string permissions;
};
