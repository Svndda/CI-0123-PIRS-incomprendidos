#pragma once
#include <iostream>
#include <string>
#include <cstdint>
#include "iNode.hpp"

/**
 * @class File
 * @brief Represents a file that encapsules metadata.
 *
 * This class provides essential file operations to perform on the entity,
 * including the metadata retrieval.
 */
class File {
 private:
  /** @brief Metadata structure containing file attributes (iNode). */
  iNode metadata;

 public:
  /** @brief Constructs a new File object with the provided data. */
  File(uint64_t id, const std::string &fileName, const std::string &permissions, uint64_t block);

  /** @brief Constructs a new File object with default metadata. */
  File();
  /** @brief Destroys the File object and releases any related resources. */
  ~File();

  /**
  * @brief Opens the file for operations.
  */
  bool open();
  /**
  * @brief Closes the file and releases associated resources.
  */
  bool close();

  bool truncate();

  bool deleteFile();

  ///> Getters
 public:
  /**
   * @brief Retrieves the file name.
   * @return File name as a string.
   */
  [[nodiscard]] std::string getName() const {return this->metadata.name;}

  /**
   * @brief Gets the file type (e.g., directory, regular file, link).
   * @return File type as a string.
   */
  [[nodiscard]] std::string getType() const {return this->metadata.type;}

  /**
  * @brief Gets the currently file state.
  * @return File state as a string.
  */
  [[nodiscard]] std::string getState() const {return this->metadata.state;}

  /**
  * @brief Gets the length of the file in bytes.
  * @return File length.
  */
  [[nodiscard]] uint64_t getLength() const {return this->metadata.length;}

  /**
  * @brief Retrieves the underlying iNode metadata.
  * @return iNode structure containing file attributes.
  */
  [[nodiscard]] iNode getMetadata() const {return this->metadata;}
};