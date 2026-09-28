#ifndef MADEIRA_CEF_COMPACT_H
#define MADEIRA_CEF_COMPACT_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Called only after the complete .text SHA256 matches a supported image.
 * Preserve opcode lengths, guard arithmetic, manager structures and bitmaps.
 * Pool::Initialize derives its usable bitmap length from the new size.
 * The four libcef exclusions are unrelated fixed-point/V8 constants. */
struct madeira_cef_compact {
    const char *name;
    uint32_t image_size, text_rva, text_size, init_rva, mask_count;
    const char *original_sha, *compact_sha;
    uint32_t exclusions[4];
};
static const struct madeira_cef_compact madeira_cef_compacts[] = {
    {"chrome_elf.dll", 0x166000, 0x1000, 0xeecc9, 0x41803, 99,
     "bae6d3c9078532846c8e76632d6e2bea1621cc44784f3db67504ca9553f0617b",
     "71a388a678744c8ce480654514b4b3030b13e6766545f96f5fbf326398592887", {0}},
    {"libcef.dll", 0xd3ca000, 0x1000, 0xb03be23, 0x1420cb3, 31706,
     "b11c92bf37c3c68e7a22f88ed307b857c056a08dc184b1ab31610fad57c1460a",
     "16c3c35ac749924ec93a8761b99c3c20d15f50bc357d29628fd312799ae116e1",
     {0x6b85fcb, 0x7a836f0, 0x86457e6, 0xae52647}}
};
static inline int madeira_cef_mask(const unsigned char *text, size_t offset,
                                  const struct madeira_cef_compact *plan)
{
    uint64_t value;
    for (unsigned i = 0; i < 4; ++i)
        if (plan->exclusions[i] && offset + plan->text_rva == plan->exclusions[i]) return 0;
    if ((text[offset] != 0x48 && text[offset] != 0x49) ||
        text[offset + 1] < 0xb8 || text[offset + 1] > 0xbf) return 0;
    memcpy(&value, text + offset + 2, sizeof(value));
    return value == UINT64_C(0xfffffffc00000000);
}
static inline int madeira_cef_compact_check(const unsigned char *text, size_t size,
                                          const struct madeira_cef_compact *plan)
{
    static const unsigned char init[] = {0x48,0xbe,0,0,0,0,4,0,0,0};
    size_t start;
    unsigned count = 0;
    if (size != plan->text_size || plan->init_rva < plan->text_rva || size < 10) return 0;
    start = plan->init_rva - plan->text_rva;
    if (start > size - 10 || memcmp(text + start, init, sizeof(init))) return 0;
    for (size_t i = 0; i <= size - 10; ++i) count += madeira_cef_mask(text, i, plan);
    return count == plan->mask_count;
}
static inline unsigned madeira_cef_compact_apply(unsigned char *text,
                                               const struct madeira_cef_compact *plan)
{
    const uint64_t mask = UINT64_C(0xffffffff80000000), pool_size = UINT64_C(0x80000000);
    unsigned count = 0;
    if (!madeira_cef_compact_check(text, plan->text_size, plan)) return 0;
    for (size_t i = 0; i <= (size_t)plan->text_size - 10; ++i)
        if (madeira_cef_mask(text, i, plan)) {
            memcpy(text + i + 2, &mask, sizeof(mask));
            count++;
        }
    memcpy(text + plan->init_rva - plan->text_rva + 2, &pool_size, sizeof(pool_size));
    return count;
}
#endif
