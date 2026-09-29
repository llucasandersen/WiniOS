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
