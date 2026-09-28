#include "../build/ntdll-unix/cef_compact.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Also accepts a local, canonical .text dump for verification against the
 * actual DLL. Proprietary Steam binaries are never uploaded into this repo. */
int main(int argc, char **argv)
{
    const uint64_t old_mask = UINT64_C(0xfffffffc00000000);
    const uint64_t new_mask = UINT64_C(0xffffffff80000000);
    const unsigned char init[] = {0x48,0xbe,0,0,0,0,4,0,0,0};
    if (argc == 4) {
        unsigned which = (unsigned)atoi(argv[1]);
        assert(which < 2);
        const struct madeira_cef_compact *p = &madeira_cef_compacts[which];
        unsigned char *b = malloc(p->text_size);
        assert(b);
        FILE *f = fopen(argv[2], "rb");
        assert(f && fread(b, 1, p->text_size, f) == p->text_size);
        assert(fgetc(f) == EOF);
        fclose(f);
        assert(madeira_cef_compact_apply(b, p) == p->mask_count);
        assert(!madeira_cef_compact_check(b, p->text_size, p));
        f = fopen(argv[3], "wb");
        assert(f && fwrite(b, 1, p->text_size, f) == p->text_size);
        fclose(f);
        free(b);
        puts("Actual DLL compact patch passed; verify output SHA256 independently");
        return 0;
    }
    unsigned char b[100], saved[100];
    struct madeira_cef_compact p = {"fixture",100,0x1000,100,0x1000,2,"","",{0x1030}};
    memset(b,0xcc,sizeof(b));
    memcpy(b,init,sizeof(init));
    b[16]=0x48; b[17]=0xb8; memcpy(b+18,&old_mask,8);
    b[32]=0x49; b[33]=0xbf; memcpy(b+34,&old_mask,8);
    b[48]=0x48; b[49]=0xb9; memcpy(b+50,&old_mask,8); /* unrelated: excluded */
    b[64]=0x48; b[65]=0x90; memcpy(b+66,&old_mask,8); /* data, not MOVABS */
    memcpy(saved,b,sizeof(b));
    assert(madeira_cef_compact_check(b,sizeof(b),&p));
    p.mask_count=3;
    assert(!madeira_cef_compact_apply(b,&p));
    assert(!memcmp(saved,b,sizeof(b))); /* mismatch must not partially patch */
    p.mask_count=2;
    assert(!madeira_cef_compact_check(b,sizeof(b)-1,&p));
    assert(madeira_cef_compact_apply(b,&p)==2);
    uint64_t value;
    memcpy(&value,b+2,8); assert(value==UINT64_C(0x80000000));
    memcpy(&value,b+18,8); assert(value==new_mask);
    memcpy(&value,b+34,8); assert(value==new_mask);
    memcpy(&value,b+50,8); assert(value==old_mask);
    memcpy(&value,b+66,8); assert(value==old_mask);
    assert(b[16]==0x48 && b[17]==0xb8 && b[32]==0x49 && b[33]==0xbf);
    assert(!madeira_cef_compact_apply(b,&p));
    /* New static masks agree with exactly 2GB, including the boundary. */
    const uint64_t base=UINT64_C(0x400000000), size=UINT64_C(0x80000000);
    assert((base & new_mask)==base);
    assert(((base+size-1) & new_mask)==base);
    assert(((base+size) & new_mask)!=base);
    assert(((base-1) & new_mask)!=base);
    puts("CEF compact: captured opcodes, masks, boundaries, exclusions, atomic refusal and idempotence passed");
}
