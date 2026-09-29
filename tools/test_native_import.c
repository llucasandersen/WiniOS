#include "../build/ntdll-unix/native_import.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void u32(unsigned char *image, size_t offset, uint32_t value) { memcpy(image + offset, &value, 4); }
static void u64(unsigned char *image, size_t offset, uint64_t value) { memcpy(image + offset, &value, 8); }
static void fixture(void)
{
    unsigned char image[2048] = {0};
    const uintptr_t parent = 0xfba0c0000ULL, alias = 0x306890000ULL, file_base = 0x180000000ULL;
    uint32_t native = 123;
    image[0]='M'; image[1]='Z'; u32(image, 0x3c, 0x80); u32(image, 0x80, 0x4550);
    image[0x98]=0x0b; image[0x99]=2; u64(image, 0x98+24, file_base);
    u32(image, 0x98+108, 16); u32(image, 0x98+112+80, 0x200); u32(image, 0x200, 0xd0);
    u32(image, 0x304, 0x400); u32(image, 0x308, 1);
    u32(image, 0x310, 0x440); u32(image, 0x334, 2);
    u32(image, 0x400, 0x501); u32(image, 0x404, 0x100);
    u32(image, 0x440, 0x600); u32(image, 0x444, 0x500);
    u32(image, 0x448, 0x650); u32(image, 0x44c, 0x580);
    uintptr_t bases[] = {file_base, parent, alias};
    for (unsigned i=0; i<3; ++i)
    {
        u64(image, 0x200+0xc8, bases[i]+0x300);
        assert(madeira_native_import(image,sizeof(image),parent,alias,0x600,&native) && native==0x500);
        assert(madeira_native_import(image,sizeof(image),parent,alias,0x650,&native) && native==0x580);
    }
    uint32_t slots[3] = {11,22,33};
    assert(!madeira_native_dispatch_slots(image,sizeof(image),parent,alias,slots) && slots[0]==11);
    u32(image,0x31c,0x700); u32(image,0x320,0x708); u32(image,0x324,0x710);
    assert(madeira_native_dispatch_slots(image,sizeof(image),parent,alias,slots));
    assert(slots[0]==0x700 && slots[1]==0x708 && slots[2]==0x710);
    u32(image,0x324,0x708);
    assert(!madeira_native_dispatch_slots(image,sizeof(image),parent,alias,slots));
    u32(image,0x324,2047);
    assert(!madeira_native_dispatch_slots(image,sizeof(image),parent,alias,slots));
    native=123;
    assert(!madeira_native_import(image,sizeof(image),parent,alias,0x601,&native) && native==123);
    u32(image,0x444,0); assert(!madeira_native_import(image,sizeof(image),parent,alias,0x600,&native));
    u32(image,0x444,2048); assert(!madeira_native_import(image,sizeof(image),parent,alias,0x600,&native));
    u32(image,0x444,0x600); assert(!madeira_native_import(image,sizeof(image),parent,alias,0x600,&native));
    u32(image,0x444,0x500); u32(image,0x334,UINT32_MAX);
    assert(!madeira_native_import(image,sizeof(image),parent,alias,0x600,&native));
    for (size_t size=0; size<sizeof(image); ++size)
        assert(!madeira_native_import(image,size,parent,alias,0x600,&native));
    puts("Native import bounds, exact-match, non-EC and pointer-form tests passed");
}
static void forwarding_fixture(void)
{
    unsigned char image[2048] = {0}; uintptr_t target = 123;
    image[0]='M'; image[1]='Z'; u32(image,0x3c,0x80); u32(image,0x80,0x4550);
    image[0x98]=0x0b; image[0x99]=2; u32(image,0x98+108,16);
    u32(image,0x98+112+96,0x500); u32(image,0x98+112+100,16);
    image[0x600]=0xff; image[0x601]=0x25; u32(image,0x602,(uint32_t)(0x500-0x606));
    u64(image,0x500,0x3069120a0ULL);
    assert(madeira_native_forward(image,sizeof(image),0x600,&target) && target==0x3069120a0ULL);
    target=123; u32(image,0x602,(uint32_t)(0x4f8-0x606));
    assert(!madeira_native_forward(image,sizeof(image),0x600,&target) && target==123);
    u32(image,0x602,(uint32_t)(0x50c-0x606));
    assert(!madeira_native_forward(image,sizeof(image),0x600,&target));
    u32(image,0x602,(uint32_t)(0x504-0x606));
    assert(!madeira_native_forward(image,sizeof(image),0x600,&target));
    u32(image,0x602,(uint32_t)(0x500-0x606)); u64(image,0x500,0);
    assert(!madeira_native_forward(image,sizeof(image),0x600,&target));
    u64(image,0x500,1); image[0x601]=0x15;
    assert(!madeira_native_forward(image,sizeof(image),0x600,&target));
    image[0x601]=0x25;
    for(size_t size=0;size<0x606;++size) assert(!madeira_native_forward(image,size,0x600,&target));
    puts("Forwarding stub IAT bounds, alignment, null and opcode tests passed");
}
int main(int argc, char **argv)
{
    fixture();
    forwarding_fixture();
    if (argc == 4 || argc == 5)
    {
        FILE *f=fopen(argv[1],"rb"); assert(f);
        assert(!fseek(f,0,SEEK_END)); long length=ftell(f); assert(length>0 && length<(64L<<20));
        rewind(f); unsigned char *image=malloc((size_t)length); assert(image);
        assert(fread(image,1,(size_t)length,f)==(size_t)length); fclose(f);
        uint32_t source=(uint32_t)strtoul(argv[2],NULL,0), expected=(uint32_t)strtoul(argv[3],NULL,0), native=0;
        if (argc == 5)
        {
            uintptr_t forwarded=0;
            assert(madeira_native_forward(image,(size_t)length,source,&forwarded));
            assert(forwarded==expected);
            printf("Actual bundled forwarding stub 0x%x resolves to IAT target 0x%x\n",source,expected);
            free(image); return 0;
        }
        assert(madeira_native_import(image,(size_t)length,0xfba0c0000ULL,0x306890000ULL,source,&native));
        assert(native==expected);
        printf("Actual bundled export 0x%x redirects to native 0x%x\n",source,native);
        free(image);
    }
    return 0;
}
