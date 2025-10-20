#ifndef LAYOUT_H
#define LAYOUT_H
#include <cstdint>
#include <cstddef>

namespace Layout {

inline constexpr uint64_t DISK_SIZE     = 1024ull * 1024ull * 1024ull;  // 1 GiB
inline constexpr uint32_t BLOCK_SIZE    = 256;                          // bytes por bloque
inline constexpr uint64_t BLOCK_COUNT   = DISK_SIZE / BLOCK_SIZE;

inline constexpr uint32_t INODE_SIZE    = 128;                          // bytes por i-nodo
inline constexpr uint32_t INODE_COUNT   = 65536;

inline constexpr uint32_t DIR_NAME_LEN  = 64;                           // nombre fijo en dir
inline constexpr uint32_t DIR_ENTRY_SIZE = DIR_NAME_LEN + 8;             // name[64] + inode_id(8)
inline constexpr uint32_t DIR_ENTRY_COUNT = 16384;

inline constexpr uint64_t SUPER_SIZE    = BLOCK_SIZE;                   // reservamos 1 bloque


inline constexpr uint64_t reservBlocks(uint64_t a, uint64_t b) {        // reservar cuantos bloques
    return (a + b - 1) / b;                                             // necesito para almacenar
}
inline constexpr uint64_t bitmapBytes(uint64_t blocks) {                // Calcular cuantos bits
    return (blocks + 7) / 8;                                            // necesito para el bitmap
}
inline constexpr uint64_t bitmapBlocks(uint64_t blocks) {               // reservar el espacio
    return reservBlocks(bitmapBytes(blocks), BLOCK_SIZE);               // para el bitmap
}
inline constexpr uint64_t inodeTableBytes() {
    return static_cast<uint64_t>(INODE_COUNT) * INODE_SIZE;
}
inline constexpr uint64_t inodeTableBlocks() {
    return reservBlocks(inodeTableBytes(), BLOCK_SIZE);
}
inline constexpr uint64_t directoryBytes() {
    return static_cast<uint64_t>(DIR_ENTRY_COUNT) * DIR_ENTRY_SIZE;
}
inline constexpr uint64_t directoryBlocks() {
    return reservBlocks(directoryBytes(), BLOCK_SIZE);
}

struct superBlock {
    uint32_t block_size = BLOCK_SIZE;
    uint64_t block_count = BLOCK_COUNT;
    uint64_t free_block_count = 0;

    uint32_t inode_size = INODE_SIZE;
    uint32_t inode_count = INODE_COUNT;
    uint32_t dir_entry_size = DIR_ENTRY_SIZE;
    uint32_t dir_entry_count = DIR_ENTRY_COUNT;

    // offsets de las estructuras
    uint64_t super_offset = 0;
    uint64_t bitmap_offset = 0;
    uint64_t inode_table_offset = 0;
    uint64_t directory_offset = 0;
    uint64_t data_area_offset = 0;
};

inline void registerOffsets(superBlock& sb) {
    sb.super_offset = 0;

    const uint64_t bmBlocks = bitmapBlocks(sb.block_count);
    const uint64_t inoBlocks = inodeTableBlocks();
    const uint64_t dirBlocks = directoryBlocks();

    sb.bitmap_offset       = sb.super_offset + SUPER_SIZE;
    sb.inode_table_offset  = sb.bitmap_offset + bmBlocks * BLOCK_SIZE;
    sb.directory_offset    = sb.inode_table_offset + inoBlocks * BLOCK_SIZE;
    sb.data_area_offset    = sb.directory_offset + dirBlocks * BLOCK_SIZE;

    const uint64_t reservedBlocks =
        reservBlocks(SUPER_SIZE, BLOCK_SIZE) + bmBlocks + inoBlocks + dirBlocks;

    if (reservedBlocks <= sb.block_count){
        sb.free_block_count = sb.block_count - reservedBlocks;
    } else {
        sb.free_block_count = 0;
    }
}
}
#endif // LAYOUT_H
