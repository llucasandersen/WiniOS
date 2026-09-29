#ifndef MADEIRA_NATIVE_IMPORT_H
#define MADEIRA_NATIVE_IMPORT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static inline int madeira_image_span(size_t size, uint32_t offset, size_t length)
{
    return offset <= size && length <= size - offset;
}
static inline uint32_t madeira_image_u32(const unsigned char *image, size_t offset)
{
    uint32_t value; memcpy(&value, image + offset, sizeof(value)); return value;
}
static inline uint64_t madeira_image_u64(const unsigned char *image, size_t offset)
{
    uint64_t value; memcpy(&value, image + offset, sizeof(value)); return value;
}

/* Resolve an exact ARM64EC export redirection, never a guessed thunk offset.
 * Accept file, parent and relocated JIT forms of the metadata pointer.
 * Require the destination to belong to an ARM64EC code range. */
static inline int madeira_native_metadata(const unsigned char *image, size_t size,
    uintptr_t parent, uintptr_t alias, uint32_t *metadata)
{
    uint32_t pe, opt, config, meta;
    uint64_t pointer, file_base;
    if (!image || !metadata || size < 64 || image[0] != 'M' || image[1] != 'Z') return 0;
    pe = madeira_image_u32(image, 0x3c);
    if (!madeira_image_span(size, pe, 24 + 0xe0) || madeira_image_u32(image, pe) != 0x4550) return 0;
    opt = pe + 24;
    if (image[opt] != 0x0b || image[opt + 1] != 2 || madeira_image_u32(image, opt + 108) <= 10) return 0;
    config = madeira_image_u32(image, opt + 112 + 10 * 8);
    if (!config || !madeira_image_span(size, config, 0xd0) || madeira_image_u32(image, config) < 0xd0) return 0;
    pointer = madeira_image_u64(image, config + 0xc8);
    file_base = madeira_image_u64(image, opt + 24);
    if (pointer >= alias && pointer - alias < size) pointer -= alias;
    else if (pointer >= parent && pointer - parent < size) pointer -= parent;
    else if (pointer >= file_base && pointer - file_base < size) pointer -= file_base;
    else return 0;
    if (pointer > UINT32_MAX) return 0;
    meta = (uint32_t)pointer;
    if (!madeira_image_span(size, meta, 56)) return 0;
    *metadata = meta;
    return 1;
}

static inline int madeira_native_import(const unsigned char *image, size_t size,
    uintptr_t parent, uintptr_t alias, uint32_t source, uint32_t *destination)
{
    uint32_t meta, table, count, lo, hi, map, map_count;
    if (!destination || !madeira_native_metadata(image, size, parent, alias, &meta)) return 0;
    table = madeira_image_u32(image, meta + 16);
    count = madeira_image_u32(image, meta + 52);
    map = madeira_image_u32(image, meta + 4);
    map_count = madeira_image_u32(image, meta + 8);
    if (!table || !map || !count || !map_count || count > size / 8 || map_count > size / 8 ||
        !madeira_image_span(size, table, (size_t)count * 8) ||
        !madeira_image_span(size, map, (size_t)map_count * 8)) return 0;
    lo = 0; hi = count;
    while (lo < hi)
    {
        uint32_t mid = lo + (hi - lo) / 2;
        uint32_t key = madeira_image_u32(image, table + (size_t)mid * 8);
        if (key < source) lo = mid + 1; else hi = mid;
    }
    if (lo == count || madeira_image_u32(image, table + (size_t)lo * 8) != source) return 0;
    uint32_t native = madeira_image_u32(image, table + (size_t)lo * 8 + 4);
    if (!native || native >= size) return 0;
    for (uint32_t i = 0; i < map_count; ++i)
    {
        uint32_t start = madeira_image_u32(image, map + (size_t)i * 8);
        uint32_t length = madeira_image_u32(image, map + (size_t)i * 8 + 4);
        uint32_t begin = start & ~3u;
        if ((start & 3u) == 1 && native >= begin && native - begin < length &&
            madeira_image_span(size, begin, length))
        {
            *destination = native;
            return 1;
        }
    }
    return 0;
}

/* Wine export forwarding stubs are x64 RIP-relative jumps into the declared
 * import address table. Follow only a bounded, aligned, nonzero IAT entry. */
static inline int madeira_native_forward(const unsigned char *image, size_t size,
    uint32_t source, uintptr_t *target)
{
    uint32_t pe, opt, iat, length;
    int32_t displacement;
    int64_t slot;
    if (!image || !target || size < 64 || image[0] != 'M' || image[1] != 'Z') return 0;
    pe = madeira_image_u32(image, 0x3c);
    if (!madeira_image_span(size, pe, 24 + 0xe0) || madeira_image_u32(image, pe) != 0x4550) return 0;
    opt = pe + 24;
    if (image[opt] != 0x0b || image[opt + 1] != 2 || madeira_image_u32(image, opt + 108) <= 12) return 0;
    iat = madeira_image_u32(image, opt + 112 + 12 * 8);
    length = madeira_image_u32(image, opt + 112 + 12 * 8 + 4);
    if (!iat || !madeira_image_span(size, iat, length) ||
        !madeira_image_span(size, source, 6) || image[source] != 0xff || image[source + 1] != 0x25) return 0;
    memcpy(&displacement, image + source + 2, 4);
    slot = (int64_t)source + 6 + displacement;
    if (slot < iat || slot - iat > length || length - (size_t)(slot - iat) < 8 || (slot & 7)) return 0;
    uintptr_t value = (uintptr_t)madeira_image_u64(image, (size_t)slot);
    if (!value) return 0;
    *target = value;
    return 1;
}

static inline int madeira_native_dispatch_slots(const unsigned char *image, size_t size,
    uintptr_t parent, uintptr_t alias, uint32_t slots[3])
{
    uint32_t meta, candidate[3];
    if (!slots || !madeira_native_metadata(image, size, parent, alias, &meta)) return 0;
    for (unsigned i = 0; i < 3; ++i)
    {
        candidate[i] = madeira_image_u32(image, meta + 28 + i * 4);
        if (!candidate[i] || !madeira_image_span(size, candidate[i], 8)) return 0;
    }
    if (candidate[0] == candidate[1] || candidate[0] == candidate[2] || candidate[1] == candidate[2]) return 0;
    memcpy(slots, candidate, sizeof(candidate));
    return 1;
}
#endif
