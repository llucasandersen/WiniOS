#ifndef MADEIRA_JUMBO_ALIGNMENT_H
#define MADEIRA_JUMBO_ALIGNMENT_H
#include <stdint.h>

static inline int madeira_jumbo_range(uint64_t base, uint64_t size, uint64_t ceiling)
{
    return base && size && base < ceiling && size <= ceiling - base;
}

/* Preserve the caller's offset without returning a range outside the host. */
static inline int madeira_jumbo_candidate(uint64_t slot, uint64_t offset,
                                          uint64_t alignment, uint64_t size,
                                          uint64_t ceiling, uint64_t *base)
{
    if (!alignment || (alignment & (alignment - 1)) || offset >= alignment ||
        slot % alignment || (offset && slot < alignment)) return 0;
    uint64_t candidate = slot;
    if (offset) {
        candidate = slot - alignment;
        if (offset > UINT64_MAX - candidate) return 0;
        candidate += offset;
    }
    if (!madeira_jumbo_range(candidate, size, ceiling))
        return 0;
    *base = candidate;
    return 1;
}
#endif
