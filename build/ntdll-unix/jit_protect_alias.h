#ifndef MADEIRA_JIT_PROTECT_ALIAS_H
#define MADEIRA_JIT_PROTECT_ALIAS_H
#include <stdint.h>

struct madeira_jit_protection_result
{
    uintptr_t address;
    int sync_parent;
};

/* Called only after a successful protection update of the parent mapping. */
static inline struct madeira_jit_protection_result madeira_jit_protection_result(
    uintptr_t live_alias, uintptr_t requested_parent, uintptr_t rounded_parent)
{
    struct madeira_jit_protection_result result;
    result.address = live_alias ? live_alias - (requested_parent - rounded_parent) : rounded_parent;
    result.sync_parent = !live_alias;
    return result;
}
#endif
