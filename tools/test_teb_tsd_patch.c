#include "../build/ntdll-unix/teb_tsd_patch.h"
#include <assert.h>
#include <stdio.h>

static void triplet(uint8_t *text, size_t word, unsigned reg)
{
    uint32_t insns[] = { 0xd53bd060u | reg,
                        0x927df000u | (reg << 5) | reg,
                        0xf9400000u | (275u << 10) | (reg << 5) | reg };
    memcpy(text + word * 4, insns, sizeof(insns));
}

int main(void)
{
    /* Exact-sized literal map: the former byte indexing runs past this map. */
    uint8_t text[1024] = {0};
    uint8_t map[sizeof(text) / 4 / 8] = {0};
    int found;
    for (unsigned reg = 0; reg < 31; ++reg)
    {
        memset(text, 0, sizeof(text));
        triplet(text, 200, reg);
        assert(madeira_retarget_teb_tsd(text, sizeof(text), map, 279 * 8, &found) == 1);
        assert(found == 1);
        uint32_t insn; memcpy(&insn, text + 202 * 4, 4);
        assert(((insn >> 10) & 4095) == 279);
        assert(madeira_retarget_teb_tsd(text, sizeof(text), map, 279 * 8, &found) == 0);
        assert(found == 1);
    }
    /* Captured fault triplet: mrs x17; and x17; ldr x17, [x17,#0x898]. */
    const uint32_t captured[] = {0xd53bd071, 0x927df231, 0xf9444e31};
    memcpy(text + 200 * 4, captured, sizeof(captured));
    for (size_t word = 200; word <= 202; ++word)
    {
        memset(map, 0, sizeof(map));
        map[word >> 3] |= 1u << (word & 7);
        assert(madeira_retarget_teb_tsd(text, sizeof(text), map, 0x8b8, &found) == 0);
        assert(found == 0);
    }
    memset(map, 0, sizeof(map));
    assert(madeira_retarget_teb_tsd(text, sizeof(text), map, 0x8b8, &found) == 1);
    uint32_t insn; memcpy(&insn, text + 202 * 4, 4);
    assert(insn == 0xf9445e31);
    memset(text, 0, sizeof(text));
    triplet(text, 200, 17);
    text[201 * 4] ^= 1; /* mismatched registers must not be patched */
    assert(!madeira_retarget_teb_tsd(text, sizeof(text), NULL, 0x8b8, &found));
    assert(!found);
    assert(!madeira_retarget_teb_tsd(text, 8, NULL, 0x8b8, &found));
    assert(!madeira_retarget_teb_tsd(text, sizeof(text), NULL, 0, &found));
    assert(!madeira_retarget_teb_tsd(text, sizeof(text), NULL, 7, &found));
    puts("TEB retarget: captured crash, all registers, packed literal bits, bounds and idempotence passed");
}
