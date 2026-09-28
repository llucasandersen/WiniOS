# Compact Steam browser pools

This downstream compatibility patch targets the exact CEF 126.0.6478.183
code shipped in this machine's Steam client and the exported device prefix.
It does not modify the stored Steam files. It runs before Wine makes the first
JIT image copy; the canonical backing is changed too, so later copies inherit it.

Both chrome_elf.dll and libcef.dll contain an independent allocator initializer.
Their regular and BRP pools each request 16GB. On the captured 63GB iOS task,
four such pools cannot coexist with Wine/FEX and the other browser cages.
The compatibility patch changes the initializer's RSI size to 2GB and adjusts
the allocator's static pool membership masks to the same size. The initializer
passes RSI to both page reservation and AddressPoolManager::Add. Guard pages,
instructions, structure sizes, and the larger backing bitmaps remain intact.
AddressPoolManager derives total_bits_ from the passed length and bounds
reservations by it. Small pools can still exhaust; this is not a RAM bypass.

Supported .text has zero base relocation entries. Original and patched SHA256
digests cover the entire section. A complete original digest, initializer
opcode and mask count must match before any write. The patched digest is checked
afterwards and handles already-patched copies. Unknown code is left unchanged
with an unsupported-version log; failed protection or post-check stops the load.

The audited libcef masks reference PartitionAddressSpace setup globals
0xc9756c0/0xc9756c8/0xc9756d0. Four occurrences of the same integer in V8/fixed-point
code are excluded, including the unwind-less routine at 0x6b85fcb. The other
unwind-less routine at 0x74f5623 uses the allocator's regular/BRP globals.
The chrome_elf setup globals are 0x13ae80/0x13ae88/0x13ae90.
Only the identified initializer size is changed; unrelated 16GB constants
throughout the DLL are preserved. No proprietary DLL is committed here.

Local tests compile the actual C patch and run it on both complete .text sections.
Their resulting SHA256 values match an independent Python implementation.
CI adds sanitizer checks for opcode matching, boundary masks, exclusions,
mismatch refusal without partial writes, and repeat application.
The normal allocator now preserves natural 2GB/4GB/8GB alignment for these
requests instead of forcing every hinted jumbo request onto 16GB slots.

The IPA does not request Extended Virtual Addressing. JIT remains necessary.
Actual Steam login/library success needs a new device run; desktop/updater,
Windows tests, and compilation alone do not establish Wine/FEX compatibility.

Allocator references:
https://chromium.googlesource.com/chromium/src/+/117.0.5938.132/base/allocator/partition_allocator/partition_address_space.cc
https://chromium.googlesource.com/chromium/src/+/refs/heads/main/base/allocator/partition_allocator/src/partition_alloc/address_pool_manager.cc
