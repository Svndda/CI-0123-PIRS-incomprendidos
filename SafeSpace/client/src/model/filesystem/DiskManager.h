#ifndef DISKMANAGER_H
#define DISKMANAGER_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <ctime>
#include <fstream>
#include "Layout.h"
#include "iNode.h"

class DiskManager
{
private:
    std::string diskPath;
    std::fstream disk;

public:
    DiskManager();
    explicit DiskManager(const std::string& path);
    ~DiskManager();
    /**
     * @brief Opens the disk for reading and writing.
     *
     * @param mode
     * @return true
     * @return false
     */
    bool openDisk(std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary);
    /**
     * @brief Closes the disk.
     *
     */
    void closeDisk();

    /**
     * @brief Check if disk is open.
     *
     * @return true if disk is open, false otherwise.
     */
    bool isOpen() const;
    /**
     * @brief Writes bytes to the disk at the specified offset.
     *
     * @param offset Offset in bytes where the data will be written.
     * @param buffer Pointer to the data to be written.
     * @param bytes Number of bytes to write.
     * @return true if the write was successful, false otherwise.
     */
    bool writeBytes(uint64_t offset, const void* buffer, size_t bytes);
    /**
     * @brief Reads bytes from the disk at the specified offset.
     *
     * @param offset Offset in bytes where the data will be read.
     * @param buffer Pointer to the buffer where the read data will be stored.
     * @param bytes Number of bytes to read.
     * @return true if the read was successful, false otherwise.
     */
    bool readBytes(uint64_t offset, void* buffer, size_t bytes);

    /**
   * @brief Resets the disk, filling it with zeros.
   */
    void resetUnity();

    /**
   * @brief Loads the bitmap from disk to memory.
   * @param superBlock Superblock containing bitmap offset information.
   * @return 0 on success, -1 on error.
   */
    int loadBitMap(std::vector<bool>& bitMap, const Layout::superBlock& superBlock);
    /**
   * @brief Saves the bitmap to disk.
   * @param bitMap The bitmap to save.
   * @param superBlock Superblock containing bitmap offset information.
   * @return true on success, false on error.
   */
    bool saveBitMap(const std::vector<bool>& bitMap, const Layout::superBlock& superBlock);
    /**
   * @brief Saves an iNode to disk at the specified offset.
   * @param disk File stream for the disk.
   * @param node The iNode to save.
   * @param offset Offset in bytes where the iNode will be saved.
   */
    bool writeInode(uint64_t offset, const iNode& inode);
    /**
   * @brief Loads an iNode from disk at the specified offset.
   * @param disk File stream for the disk.
   * @param offset Offset in bytes where the iNode is stored.
   * @return The loaded iNode.
   */
    bool readInode(uint64_t offset, iNode& outInode);

};
#endif // DISKMANAGER_H
