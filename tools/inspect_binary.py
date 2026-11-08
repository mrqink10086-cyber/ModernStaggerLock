import io
import struct
from pathlib import Path

import capstone
import pefile


def address_library(path):
    data = io.BytesIO(Path(path).read_bytes())
    def read(fmt):
        return struct.unpack('<' + fmt, data.read(struct.calcsize('<' + fmt)))[0]
    assert read('I') in (1, 2)
    data.read(16)
    data.read(read('I'))
    pointer_size, count = read('I'), read('I')
    result = {}
    previous_id = previous_offset = 0
    def value(kind, previous):
        if kind == 0:
            return read('Q')
        if kind == 1:
            return previous + 1
        if kind in (2, 3, 4, 5):
            delta = read('B' if kind < 4 else 'H')
            return previous + delta if kind % 2 == 0 else previous - delta
        return read('H' if kind == 6 else 'I')
    for _ in range(count):
        kind = read('B')
        current_id = value(kind & 15, previous_id)
        high = kind >> 4
        offset = value(high & 7, previous_offset // pointer_size if high & 8 else previous_offset)
        if high & 8:
            offset *= pointer_size
        result[current_id] = offset
        previous_id, previous_offset = current_id, offset
    return result


class Binary:
    def __init__(self, path, library):
        self.pe = pefile.PE(path, fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.ids = address_library(library)
        self.reverse = {offset: key for key, offset in self.ids.items()}
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)

    def bytes(self, rva, size):
        return self.pe.get_data(rva, size)

    def pointer(self, rva):
        return struct.unpack('<Q', self.bytes(rva, 8))[0] - self.base

    def dump(self, rva, size):
        for ins in self.md.disasm(self.bytes(rva, size), self.base + rva):
            print(f'{ins.address-self.base:08X} {ins.bytes.hex(" "):30} {ins.mnemonic:8} {ins.op_str}')


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('exe')
    parser.add_argument('library')
    parser.add_argument('rva', type=lambda value: int(value, 0))
    parser.add_argument('--size', type=lambda value: int(value, 0), default=0x100)
    args = parser.parse_args()
    Binary(args.exe, args.library).dump(args.rva, args.size)
