"""Map the actual bundled DLL for the production native-import resolver test."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
raw = (Path(__file__).resolve().parents[1] / 'app/Madeira/arm64ec-windows/kernelbase.dll').read_bytes()
u32 = lambda offset: struct.unpack_from('<I', raw, offset)[0]
pe = u32(0x3c)
assert raw[:2] == b'MZ' and raw[pe:pe+4] == b'PE\0\0'
sections, optional_size = struct.unpack_from('<H', raw, pe+6)[0], struct.unpack_from('<H', raw, pe+20)[0]
optional = pe + 24
size, headers = u32(optional+56), u32(optional+60)
assert 0 < headers <= size < 64 * 1024 * 1024
image = bytearray(size)
image[:headers] = raw[:headers]
for index in range(sections):
    section = optional + optional_size + index * 40
    address, length, start = u32(section+12), u32(section+16), u32(section+20)
    assert address + length <= size and start + length <= len(raw)
    image[address:address+length] = raw[start:start+length]
with tempfile.TemporaryDirectory(prefix='madeira-native-import-') as folder:
    mapped = Path(folder) / 'kernelbase.image'
    mapped.write_bytes(image)
    # Exact source/destination from the build 10 allocation fault and the
    # bundled module's own ARM64EC redirection metadata.
    subprocess.run([sys.argv[1], str(mapped), '0x84ee0', '0x40200'], check=True)
    subprocess.run([sys.argv[1], str(mapped), '0x820a0', '0x23d98'], check=True)

# Reproduce kernel32's GetSystemTimePreciseAsFileTime forwarding path from
# build 11's fault, using the actual DLL and a relocated IAT value.
raw = (Path(__file__).resolve().parents[1] / 'app/Madeira/arm64ec-windows/kernel32.dll').read_bytes()
pe = u32(0x3c); optional = pe + 24
sections = struct.unpack_from('<H',raw,pe+6)[0]
optional_size = struct.unpack_from('<H',raw,pe+20)[0]
size, headers = u32(optional+56), u32(optional+60)
image = bytearray(size); image[:headers] = raw[:headers]
for index in range(sections):
    section = optional + optional_size + index * 40
    address, length, start = u32(section+12), u32(section+16), u32(section+20)
    assert address + length <= size and start + length <= len(raw)
    image[address:address+length] = raw[start:start+length]
assert image[0x36290:0x36292] == b'\xff\x25'
slot = 0x36296 + struct.unpack_from('<i',image,0x36292)[0]
assert slot == 0x50b00
struct.pack_into('<Q',image,slot,0x12345678)
with tempfile.TemporaryDirectory(prefix='madeira-native-forward-') as folder:
    mapped = Path(folder) / 'kernel32.image'; mapped.write_bytes(image)
    subprocess.run([sys.argv[1],str(mapped),'0x36290','0x12345678','forward'],check=True)
