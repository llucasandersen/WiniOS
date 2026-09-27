import shutil
import re
import os

print("Patching FEX CMakeLists.txt...")
with open("FEX/CMakeLists.txt", "r", encoding="utf-8", errors="ignore") as f:
    s = f.read()
s = s.replace("string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} processor)", "set(CMAKE_SYSTEM_PROCESSOR \"arm64\")\nstring(TOLOWER \"${CMAKE_SYSTEM_PROCESSOR}\" processor)")
with open("FEX/CMakeLists.txt", "w", encoding="utf-8") as f:
    f.write(s)

print("Patching AtomicRefPolyfill...")
shutil.copyfile(".github/AtomicRefPolyfill.h", "FEX/FEXCore/include/FEXCore/Utils/AtomicRefPolyfill.h")
for p in ["FEX/FEXCore/include/FEXCore/Utils/SpinWaitLock.h", "FEX/FEXCore/include/FEXCore/Utils/WritePriorityMutex.h", "FEX/FEXCore/include/FEXCore/Utils/SHMStats.h"]:
    with open(p, "r", encoding="utf-8", errors="ignore") as f:
        c = f.read()
    if "AtomicRefPolyfill.h" not in c:
        c = "#include <FEXCore/Utils/AtomicRefPolyfill.h>\n" + c
        with open(p, "w", encoding="utf-8") as f:
            f.write(c)

print("Patching Core.cpp...")
core_cpp = "FEX/FEXCore/Source/Interface/Core/Core.cpp"
with open(core_cpp, "r", encoding="utf-8", errors="ignore") as f:
    cc = f.read()
old_block = "  /* iOS-Madeira ml316: report ExitToX64 FFS bypasses"
if old_block in cc and "#ifdef FEX_IOS_HOST\n  /* iOS-Madeira ml316" not in cc:
    cc = cc.replace(old_block, "#ifdef FEX_IOS_HOST\n" + old_block)
    close_block = "IosCbEntryLog[4], IosCbEntryLog[5], IosCbEntryLog[7]);\n    }\n  }"
    cc = cc.replace(close_block, close_block + "\n#endif")
    with open(core_cpp, "w", encoding="utf-8") as f:
        f.write(cc)

print("Patching Arm64.cpp...")
arm64_cpp = "FEX/FEXCore/Source/Utils/ArchHelpers/Arm64.cpp"
with open(arm64_cpp, "r", encoding="utf-8", errors="ignore") as f:
    ac = f.read()
if "VirtualQuery" in ac:
    pat = r"static void IosLogUnimplementedCASPAL\([^{]*\{[\s\S]*?\n\}"
    rep = (
        "static void IosLogUnimplementedCASPAL(uint32_t Size, uint64_t* GPRs, uint32_t AddressReg) {\n"
        "  static int reports = 0;\n"
        "  if (Size == 0 || (GPRs[AddressReg] & 15) == 0 || reports >= 8) {\n"
        "    return;\n"
        "  }\n"
        "  reports++;\n"
        "  LogMan::Msg::EFmt(\"[caspal128] MISALIGNED-UNSUPPORTED Size={} addrReg=x{} addr={:#x} misalign={} crosses16B={}\",\n"
        "                    Size, AddressReg, GPRs[AddressReg], GPRs[AddressReg] & 15,\n"
        "                    (GPRs[AddressReg] & 15) ? \"yes\" : \"no\");\n"
        "}"
    )
    ac = re.sub(pat, rep, ac)
    with open(arm64_cpp, "w", encoding="utf-8") as f:
        f.write(ac)

print("Patching Apple allocator fallback...")
allocator_cpp = "FEX/FEXCore/Source/Utils/AllocatorHooks.cpp"
with open(allocator_cpp, "r", encoding="utf-8", errors="ignore") as f:
    allocator_source = f.read()
old_guard = "size_t malloc_usable_size(void* ptr) {\n  IOS_RPM_GUARD();\n#ifdef __APPLE__"
new_guard = "size_t malloc_usable_size(void* ptr) {\n#ifdef __APPLE__"
if old_guard in allocator_source:
    allocator_source = allocator_source.replace(old_guard, new_guard)
    with open(allocator_cpp, "w", encoding="utf-8") as f:
        f.write(allocator_source)

print("Patching wine/configure...")
if os.path.exists("wine/configure"):
    with open("wine/configure", "r", encoding="utf-8", errors="ignore") as f:
        wc = f.read()
    old_pe_check = 'test "x$PE_ARCHS" != x || as_fn_error $? "PE cross-compilation is required'
    if old_pe_check in wc:
        wc = wc.replace(old_pe_check, '# bypassed PE check: ' + old_pe_check)
        with open("wine/configure", "w", encoding="utf-8") as f:
            f.write(wc)
print("Patching iOS rusage diagnostic for Xcode 16 SDK...")
server_ios_cpp = "build/ntdll-unix/server_ios.c"
with open(server_ios_cpp, "r", encoding="utf-8", errors="ignore") as f:
    server_ios_source = f.read()
server_ios_source = server_ios_source.replace(" run=%.0f pgw=%.1f GHz", " run=%.0f GHz")
server_ios_source = server_ios_source.replace("XP_MS( ru.ri_runnable_time - pru.ri_runnable_time ), XP_MS( ru.ri_page_wait_time_mach - pru.ri_page_wait_time_mach ),", "XP_MS( ru.ri_runnable_time - pru.ri_runnable_time ),")
with open(server_ios_cpp, "w", encoding="utf-8") as f:
    f.write(server_ios_source)

print("Patching Wine sync header order...")
sync_cpp = "wine/dlls/ntdll/unix/sync.c"
with open(sync_cpp, "r", encoding="utf-8", errors="ignore") as f:
    sync_source = f.read()
old_headers = '#include "../../../../build/madeira_cfg.h"   /* ml1122: before the Wine headers, which ban strncpy by macro */\n#include "config.h"'
new_headers = '#include "config.h"\n#include "../../../../build/madeira_cfg.h"   /* ml1122: before Wine public headers */'
if old_headers in sync_source:
    sync_source = sync_source.replace(old_headers, new_headers)
    with open(sync_cpp, "w", encoding="utf-8") as f:
        f.write(sync_source)

print("Patching rpmalloc.c for 454GB regime...")
rpmalloc_c = "FEX/External/rpmalloc/rpmalloc/rpmalloc.c"
if os.path.exists(rpmalloc_c):
    with open(rpmalloc_c, "r", encoding="utf-8", errors="ignore") as f:
        rc = f.read()
    if "{0x7080000000ULL, 0x7170000000ULL}" not in rc:
        old_cand = "{0x7c00000000ULL, 0x7fffffffffULL},"
        new_cand = old_cand + "\n\t\t{0x7080000000ULL, 0x7170000000ULL},"
        rc = rc.replace(old_cand, new_cand)
        with open(rpmalloc_c, "w", encoding="utf-8") as f:
            f.write(rc)
    print("rpmalloc.c patched!")

print("All patching complete!")
