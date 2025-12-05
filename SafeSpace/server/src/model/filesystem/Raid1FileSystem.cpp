#include "Raid1FileSystem.h"
#include <iostream>

Raid1FileSystem::Raid1FileSystem(const std::string& primaryPath, const std::string& mirrorPath)
    : primary(primaryPath), mirror(mirrorPath),
      primaryHealthy(primary.isValid()), mirrorHealthy(mirror.isValid()) {
    if (!primaryHealthy) {
        std::cerr << "[RAID1] Invalid primary, mirror will be used if available.\n";
    }
    if (!mirrorHealthy) {
        std::cerr << "[RAID1] Mirror not valid, operating only with primary if available.\n";
    }
}

Raid1FileSystem::~Raid1FileSystem() {}

bool Raid1FileSystem::isValid() const {
    std::lock_guard<std::mutex> lock(mtx);
    return primaryHealthy || mirrorHealthy;
}

const std::vector<DirEntry>& Raid1FileSystem::getDirectory() const {
    std::lock_guard<std::mutex> lock(mtx);
    return activeFS().getDirectory();
}

void Raid1FileSystem::listFiles() const {
    std::lock_guard<std::mutex> lock(mtx);
    activeFS().listFiles();
}

const FileSystem& Raid1FileSystem::activeFS() const {
    return (primaryHealthy ? primary : mirror);
}
FileSystem& Raid1FileSystem::activeFS() {
    return (primaryHealthy ? primary : mirror);
}

bool Raid1FileSystem::ensurePrimaryHealthy() {
    primaryHealthy = primary.isValid();
    return primaryHealthy;
}

bool Raid1FileSystem::ensureMirrorHealthy() {
    mirrorHealthy = mirror.isValid();
    return mirrorHealthy;
}

void Raid1FileSystem::tryRecoverPrimary() {
    std::lock_guard<std::mutex> lock(mtx);
    primaryHealthy = primary.isValid();
    if (primaryHealthy) {
        std::cout << "[RAID1] Primary recovered.\n";
    }
}

int Raid1FileSystem::find(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mtx);
    if (primaryHealthy) {
        int id = primary.find(name);
        if (id >= 0) return id;
        if (mirrorHealthy) return mirror.find(name);
        return -1;
    }
    if (mirrorHealthy) return mirror.find(name);
    return -1;
}

int Raid1FileSystem::create(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    int createdPrimary = -1;
    if (ensurePrimaryHealthy()) {
        createdPrimary = primary.create(name);
        if (createdPrimary < 0) {
            std::cerr << "[RAID1] Failure to create in primary, will attempt in mirror.\n";
            primaryHealthy = primary.isValid();
        }
    }
    int createdMirror = -1;
    if (ensureMirrorHealthy()) {
        if (mirror.find(name) < 0) {
            createdMirror = mirror.create(name);
            if (createdMirror < 0) {
                std::cerr << "[RAID1] Failure to create mirror.\n";
                mirrorHealthy = mirror.isValid();
            }
        } else {
            createdMirror = mirror.find(name);
        }
    }

    replicateCreate(name, createdPrimary);

    if (createdPrimary >= 0) return createdPrimary;
    if (createdMirror >= 0) return createdMirror;
    return -1;
}

bool Raid1FileSystem::write(const std::string& name, const std::string& data) {
    std::lock_guard<std::mutex> lock(mtx);

    bool wrotePrimary = false;
    if (ensurePrimaryHealthy()) {
        if (primary.openFile(name) == 0 || true) {
            wrotePrimary = primary.write(name, data);
            primary.closeFile(name);
            if (!wrotePrimary) {
                std::cerr << "[RAID1] Error writing to primary. Will attempt on mirror..\n";
                primaryHealthy = primary.isValid();
            }
        }
    }

    if (wrotePrimary) {
        if (ensureMirrorHealthy()) {
            if (mirror.find(name) < 0) {
                if (mirror.create(name) < 0) {
                    std::cerr << "[RAID1] The mirror file for replication could not be created..\n";
                    mirrorHealthy = mirror.isValid();
                    return true; // primary already persisted
                }
            }
            if (mirror.openFile(name) == 0 || true) {
                bool wroteMirror = mirror.write(name, data);
                mirror.closeFile(name);
                if (!wroteMirror) {
                    std::cerr << "[RAID1] Error mirroring after success on primary.\n";
                    mirrorHealthy = mirror.isValid();
                }
            }
        }
        return true;
    }

    if (!wrotePrimary) {
        if (ensureMirrorHealthy()) {
            if (mirror.find(name) < 0) {
                if (mirror.create(name) < 0) {
                    std::cerr << "[RAID1] Failure to create mirror after primary failure.\n";
                    mirrorHealthy = mirror.isValid();
                    return false;
                }
            }
            if (mirror.openFile(name) == 0 || true) {
                bool wroteMirror = mirror.write(name, data);
                mirror.closeFile(name);
                if (!wroteMirror) {
                    std::cerr << "[RAID1] Mirror writing error.\n";
                    mirrorHealthy = mirror.isValid();
                    return false;
                }
                return true;
            }
        }
        return false;
    }

    return true;
}

std::string Raid1FileSystem::read(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    if (ensurePrimaryHealthy()) {
        if (primary.find(name) >= 0) {
            if (primary.openFile(name) == 0 || true) {
                auto out = primary.read(name);
                primary.closeFile(name);
                if (!out.empty() || mirrorHealthy == false) {
                    return out;
                }
            }
        }
    }
    if (ensureMirrorHealthy()) {
        if (mirror.find(name) >= 0) {
            if (mirror.openFile(name) == 0 || true) {
                auto out = mirror.read(name);
                mirror.closeFile(name);
                return out;
            }
        }
    }
    return {};
}

bool Raid1FileSystem::remove(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    bool removedPrimary = false;
    if (ensurePrimaryHealthy()) {
        removedPrimary = primary.remove(name);
        if (!removedPrimary) {
            primaryHealthy = primary.isValid();
        }
    }
    bool removedMirror = false;
    if (ensureMirrorHealthy()) {
        removedMirror = mirror.remove(name);
        if (!removedMirror) {
            mirrorHealthy = mirror.isValid();
        }
    }
    replicateRemove(name, removedPrimary);
    return removedPrimary || removedMirror;
}

int Raid1FileSystem::openFile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    if (ensurePrimaryHealthy()) {
        if (primary.find(name) >= 0) {
            return primary.openFile(name);
        }
    }
    if (ensureMirrorHealthy()) {
        if (mirror.find(name) >= 0) {
            return mirror.openFile(name);
        }
    }
    return -1;
}

int Raid1FileSystem::closeFile(const std::string& name) {
    std::lock_guard<std::mutex> lock(mtx);
    int rc = 0;
    if (primaryHealthy) {
        rc = primary.closeFile(name);
    }
    if (mirrorHealthy) {
        mirror.closeFile(name);
    }
    return rc;
}

bool Raid1FileSystem::replicateCreate(const std::string& name, int createdInPrimary) {
    if (createdInPrimary >= 0 && mirrorHealthy) {
        if (mirror.find(name) < 0) {
            int createdMirror = mirror.create(name);
            if (createdMirror < 0) {
                std::cerr << "[RAID1] Unable to replicate mirror create.\n";
                mirrorHealthy = mirror.isValid();
                return false;
            }
        }
    }
    return true;
}

bool Raid1FileSystem::replicateRemove(const std::string& name, bool removedInPrimary) {
    if (removedInPrimary && mirrorHealthy) {
        if (mirror.find(name) >= 0) {
            mirror.remove(name);
        }
    }
    return true;
}