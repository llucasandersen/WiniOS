#include "../build/ntdll-unix/jumbo_alignment.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    const uint64_t align = 0x400000000ULL, ceiling = 0xfc0000000ULL;
    uint64_t base = 0;
    /* Captured hole begins at 0x370000000; the kernel pick was unaligned.
     * A real aligned 16GB grant fits at 0x400000000, avoiding 32GB overreserve. */
    assert(madeira_jumbo_candidate(align, 0, align, align, ceiling, &base));
    assert(base == align && base >= 0x370000000ULL && base + align <= 0xb80000000ULL);
    assert(madeira_jumbo_candidate(align, align - 0x10000, align, align, ceiling, &base));
    assert(base == align - 0x10000); /* guard-before-pool layout */
    assert(!madeira_jumbo_candidate(align * 3, 0, align, align, ceiling, &base));
    assert(!madeira_jumbo_candidate(0x7c00000000ULL, 0, align, align, ceiling, &base));
    assert(madeira_jumbo_candidate(align * 2, 0, align, 0x200000000ULL, ceiling, &base));
    assert(base == align * 2); /* aligned 8GB cage */
    assert(!madeira_jumbo_candidate(align, 0, 0, align, ceiling, &base));
    assert(!madeira_jumbo_candidate(align + 1, 0, align, align, ceiling, &base));
    assert(!madeira_jumbo_candidate(align, align, align, align, ceiling, &base));
    assert(!madeira_jumbo_candidate(align, 0, align, UINT64_MAX, ceiling, &base));
    puts("Jumbo alignment: captured constrained hole, guard offset, cage, ceiling and overflow passed");
}
