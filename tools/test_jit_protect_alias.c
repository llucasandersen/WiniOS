#include "../build/ntdll-unix/jit_protect_alias.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Reproduce WriteModuleRVA's protect/write/restore sequence with distinct
 * parent and live images. All three native dispatchers must survive, along
 * with unrelated globals already initialized in the live image. */
static void regression(uintptr_t page_size)
{
    uint64_t parent[8] = { 0x30e4fa200, 0x30e4fa200, 0x30e4fa200, 0, 0, 0, 0, 0 };
    uint64_t live[8];
    const uintptr_t parent_page = 0xfb2c90000ULL;
    const uintptr_t alias_page = 0x30eac0000ULL;
    memcpy(live, parent, sizeof(live));
    live[7] = 0xbc20f0000ULL;
    for (unsigned slot = 0; slot < 3; ++slot)
    {
        uintptr_t offset = slot * sizeof(uint64_t);
        struct madeira_jit_protection_result writable = madeira_jit_protection_result(
            alias_page + offset, parent_page + offset, parent_page);
        assert(writable.address == alias_page && !writable.sync_parent);
        if (writable.sync_parent) memcpy(live, parent, sizeof(live));
        live[slot] = 0x30e86a000ULL;
        /* NtProtect returned a rounded range, reused on the restore call. */
        struct madeira_jit_protection_result restored = madeira_jit_protection_result(
            writable.address, parent_page, parent_page);
        assert(restored.address == alias_page && !restored.sync_parent);
        if (restored.sync_parent) memcpy(live, parent, sizeof(live));
    }
    for (unsigned slot = 0; slot < 3; ++slot) assert(live[slot] == 0x30e86a000ULL);
    assert(live[7] == 0xbc20f0000ULL);
    /* Loader writes to the parent still propagate to its JIT image. */
    parent[3] = 0x30e507368ULL;
    struct madeira_jit_protection_result loader = madeira_jit_protection_result(
        0, parent_page + 24, parent_page);
    assert(loader.sync_parent && loader.address == parent_page);
    if (loader.sync_parent) memcpy(live, parent, sizeof(live));
    assert(live[3] == parent[3]);
    /* An unaligned request crossing a page boundary returns the corresponding
     * alias page, for Wine's 4 KB and Apple's 16 KB page granularities. */
    uintptr_t offset = page_size + 13;
    struct madeira_jit_protection_result crossing = madeira_jit_protection_result(
        alias_page + offset, parent_page + offset, parent_page + page_size);
    assert(crossing.address == alias_page + page_size && !crossing.sync_parent);
}

int main(void)
{
    regression(4096);
    regression(16384);
    puts("JIT alias protection regression passed: dispatcher writes and globals preserved; loader sync retained");
    return 0;
}
