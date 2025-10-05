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
    /**
     * @brief Unique identifier for the iNode.
     * 
     */
    uint64_t inode_id;
    /**
     * @brief Group identifier for the iNode.
     * 
     */
    uint32_t group_id;
    /**
     * @brief Size of the file in bytes.
     * 
     */
    uint64_t size_bytes;
    /**
     * @brief Creation time of the file.
     * 
     */
    uint64_t ctime;
    /**
     * @brief Last modified time of the file.
     * 
     */
    uint64_t mtime;
    /**
     * @brief Last access time of the file.
     * 
     */
    uint64_t atime;
    /**
     * @brief Number of blocks used by the file.
     * 
     */
    uint32_t blocks_used;
    /**
     * @brief File mode (permissions and type).
     * 
     */
    uint32_t mode;

    /**
     * @brief Direct block pointers for the file.
     * 
     */
    uint32_t direct[10];
    /**
     * @brief Single indirect block pointer for the file.
     * 
     */
    uint32_t indirect1;
    /**
     * @brief Flags for the iNode open, close.
     * 
     */
    uint32_t flags;
    /**
     * @brief Padding to align the structure to 128 bytes.
     * 
     */
    uint8_t  pad[8];
};
