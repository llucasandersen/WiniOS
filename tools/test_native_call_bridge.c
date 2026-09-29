#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "../build/ntdll-unix/native_call_bridge.h"

__attribute__((noinline, used)) uintptr_t ios_fex_native_call_target(uintptr_t target)
{
    assert(target == 0xaabb);
    __asm__ volatile(
        "mov x12, #0\n"
        "mov x13, #0\n"
        "mov x14, #0\n"
        "mov x15, #0\n"
        "movi v0.16b, #0\n"
        "movi v1.16b, #0\n"
        "movi v2.16b, #0\n"
        "movi v3.16b, #0\n"
        "movi v4.16b, #0\n"
        "movi v5.16b, #0\n"
        "movi v6.16b, #0\n"
        "movi v7.16b, #0\n"
        "movi v8.16b, #0\n"
        "movi v9.16b, #0\n"
        "movi v10.16b, #0\n"
        "movi v11.16b, #0\n"
        "movi v12.16b, #0\n"
        "movi v13.16b, #0\n"
        "movi v14.16b, #0\n"
        "movi v15.16b, #0\n"
        "movi v16.16b, #0\n"
        "movi v17.16b, #0\n"
        "movi v18.16b, #0\n"
        "movi v19.16b, #0\n"
        "movi v20.16b, #0\n"
        "movi v21.16b, #0\n"
        "movi v22.16b, #0\n"
        "movi v23.16b, #0\n"
        "movi v24.16b, #0\n"
        "movi v25.16b, #0\n"
        "movi v26.16b, #0\n"
        "movi v27.16b, #0\n"
        "movi v28.16b, #0\n"
        "movi v29.16b, #0\n"
        "movi v30.16b, #0\n"
        "movi v31.16b, #0\n"
        : : : "x12", "x13", "x14", "x15", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
    return 0xccdd;
}

__attribute__((naked, noinline)) static void capture(uint64_t *result)
{
    __asm__ volatile(
        "stp x29, x30, [sp, #-32]!\n"
        "stp x19, x20, [sp, #16]\n"
        "mov x19, x0\n"
        "mov x0, #256\n"
        "mov x1, #257\n"
        "mov x2, #258\n"
        "mov x3, #259\n"
        "mov x4, #260\n"
        "mov x5, #261\n"
        "mov x6, #262\n"
        "mov x7, #263\n"
        "mov x8, #264\n"
        "mov x9, #265\n"
        "mov x10, #266\n"
        "mov x11, #0xaabb\n"
        "mov x12, #268\n"
        "mov x13, #269\n"
        "mov x14, #270\n"
        "mov x15, #271\n"
        "movi v0.16b, #1\n"
        "movi v1.16b, #2\n"
        "movi v2.16b, #3\n"
        "movi v3.16b, #4\n"
        "movi v4.16b, #5\n"
        "movi v5.16b, #6\n"
        "movi v6.16b, #7\n"
        "movi v7.16b, #8\n"
        "movi v8.16b, #9\n"
        "movi v9.16b, #10\n"
        "movi v10.16b, #11\n"
        "movi v11.16b, #12\n"
        "movi v12.16b, #13\n"
        "movi v13.16b, #14\n"
        "movi v14.16b, #15\n"
        "movi v15.16b, #16\n"
        "movi v16.16b, #17\n"
        "movi v17.16b, #18\n"
        "movi v18.16b, #19\n"
        "movi v19.16b, #20\n"
        "movi v20.16b, #21\n"
        "movi v21.16b, #22\n"
        "movi v22.16b, #23\n"
        "movi v23.16b, #24\n"
        "movi v24.16b, #25\n"
        "movi v25.16b, #26\n"
        "movi v26.16b, #27\n"
        "movi v27.16b, #28\n"
        "movi v28.16b, #29\n"
        "movi v29.16b, #30\n"
        "movi v30.16b, #31\n"
        "movi v31.16b, #32\n"
        "bl _madeira_native_call_checker\n"
        "stp x0, x1, [x19, #0]\n"
        "stp x2, x3, [x19, #16]\n"
        "stp x4, x5, [x19, #32]\n"
        "stp x6, x7, [x19, #48]\n"
        "stp x8, x9, [x19, #64]\n"
        "stp x10, x11, [x19, #80]\n"
        "stp x12, x13, [x19, #96]\n"
        "stp x14, x15, [x19, #112]\n"
        "stp q0, q1, [x19, #128]\n"
        "stp q2, q3, [x19, #160]\n"
        "stp q4, q5, [x19, #192]\n"
        "stp q6, q7, [x19, #224]\n"
        "stp q8, q9, [x19, #256]\n"
        "stp q10, q11, [x19, #288]\n"
        "stp q12, q13, [x19, #320]\n"
        "stp q14, q15, [x19, #352]\n"
        "stp q16, q17, [x19, #384]\n"
        "stp q18, q19, [x19, #416]\n"
        "stp q20, q21, [x19, #448]\n"
        "stp q22, q23, [x19, #480]\n"
        "stp q24, q25, [x19, #512]\n"
        "stp q26, q27, [x19, #544]\n"
        "stp q28, q29, [x19, #576]\n"
        "stp q30, q31, [x19, #608]\n"
        "ldp x19, x20, [sp, #16]\n"
        "ldp x29, x30, [sp], #32\n"
        "ret\n");
}

int main(void)
{
    uint64_t result[80] = {0};
    capture(result);
    for (unsigned i = 0; i < 16; ++i)
        if (i != 9 && i != 11) assert(result[i] == 0x100 + i);
    assert(result[11] == 0xccdd);
    const unsigned char *vectors = (const unsigned char *)(result + 16);
    for (unsigned i = 0; i < 32; ++i)
        for (unsigned byte = 0; byte < 16; ++byte) assert(vectors[i * 16 + byte] == i + 1);
    puts("Native check-call ABI passed: argument registers and all 32 SIMD registers preserved");
    return 0;
}
