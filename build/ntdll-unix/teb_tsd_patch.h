#ifndef MADEIRA_TEB_TSD_PATCH_H
#define MADEIRA_TEB_TSD_PATCH_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* The loader's literal map contains one BIT per instruction, not one byte. */
static inline int madeira_text_is_data(const uint8_t *map, size_t word)
{
    return map && (map[word >> 3] & (1u << (word & 7)));
}

/* Retarget only the exact mrs/and/ldr TEB triplet, through the text RW alias. */
static inline int madeira_retarget_teb_tsd(uint8_t *text, size_t size,
                                         const uint8_t *map, uint32_t offset,
                                         int *found)
{
    int changed = 0;
    *found = 0;
    if (!offset || (offset & 7) || offset > 0x7ff8) return 0;
    const uint32_t wanted = (offset / 8) << 10;
    for (size_t i = 0; i + 12 <= size; i += 4)
    {
        uint32_t a, b, c;
        if (madeira_text_is_data(map, i / 4) ||
            madeira_text_is_data(map, i / 4 + 1) ||
            madeira_text_is_data(map, i / 4 + 2)) continue;
        memcpy(&a, text + i, 4);
        memcpy(&b, text + i + 4, 4);
        memcpy(&c, text + i + 8, 4);
        if ((a & 0xffffffe0u) != 0xd53bd060u) continue;
        const unsigned reg = a & 0x1f;
        if (b != (0x927df000u | (reg << 5) | reg)) continue;
        if ((c & 0xffc003ffu) != (0xf9400000u | (reg << 5) | reg)) continue;
        ++*found;
        if ((c & 0x003ffc00u) == wanted) continue;
        c = (c & ~0x003ffc00u) | wanted;
        memcpy(text + i + 8, &c, 4);
        ++changed;
    }
    return changed;
}
#endif
