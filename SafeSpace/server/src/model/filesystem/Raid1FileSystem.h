#ifndef RAID1FILESYSTEM_H
#define RAID1FILESYSTEM_H

#pragma once
#include "FileSystem.h"
#include <mutex>
#include <string>
#include <vector>

/**
 * @class Raid1FileSystem
 * @brief Implements RAID 1 (mirroring) on two FileSystem instances.
 */
class Raid1FileSystem {
 public:
    Raid1FileSystem(const std::string& primaryPath, const std::string& mirrorPath);
    ~Raid1FileSystem();

    // File operations
    int create(const std::string& name);
    bool write(const std::string& name, const std::string& data);
    std::string read(const std::string& name);
    bool remove(const std::string& name);
    int find(const std::string& name) const;

    // Opening/closing control
    int openFile(const std::string& name);
    int closeFile(const std::string& name);

    // Status and utilities
    bool isValid() const;
    void listFiles() const;

    // Active system directory
    const std::vector<DirEntry>& getDirectory() const;

    // Manual recovery of the primary
    void tryRecoverPrimary();

 private:
    FileSystem primary;
    FileSystem mirror;
    mutable std::mutex mtx;

    bool primaryHealthy;
    bool mirrorHealthy;

    const FileSystem& activeFS() const;
    FileSystem& activeFS();

    bool ensurePrimaryHealthy();
    bool ensureMirrorHealthy();

    bool replicateCreate(const std::string& name, int createdInPrimary);
    bool replicateRemove(const std::string& name, bool removedInPrimary);
};

#endif // RAID1FILESYSTEM_H