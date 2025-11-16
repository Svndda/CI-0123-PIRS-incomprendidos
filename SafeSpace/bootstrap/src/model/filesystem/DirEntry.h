#ifndef DIRENTRY_H
#define DIRENTRY_H
#include <cstdint>
#include "Layout.h"


struct DirEntry {
    char     name[Layout::DIR_NAME_LEN]; // UTF-8 truncado
    uint64_t inode_id;                   // 0 = libre
};
#endif // DIRENTRY_H
