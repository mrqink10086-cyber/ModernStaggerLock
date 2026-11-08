import argparse
import json
import re
import struct
from pathlib import Path

from inspect_binary import Binary


def audit(exe, library, commonlib, runtime):
    binary = Binary(exe, library)
    ae = runtime == '1170'
    column = 1 if ae else 0
    header = (Path(commonlib) / 'include/RE/Offsets_VTABLE.h').read_text()
    records = []
    for name, table, table_index, expected in [
        ('Character Update', 'Character', 0, (0x5D6FB0, 0x667D40)),
        ('Player Update', 'PlayerCharacter', 0, (0x69E580, 0x732660)),
        ('Character NotifyAnimationGraph', 'Character', 3, (0x60F240, 0x6A35F0)),
        ('Player NotifyAnimationGraph', 'PlayerCharacter', 3, (0x60F240, 0x6A35F0)),
        ('Character ProcessEvent', 'Character', 2, (0x60B470, 0x69F450)),
        ('JumpHandler CanProcess', 'JumpHandler', 0, (0x70A670, 0x7A08A0)),
    ]:
        line = next(line for line in header.splitlines() if f' VTABLE_{table}' + '{' in line)
        ids = re.findall(r'VariantID\((\d+), (\d+),', line)
        id = int(ids[table_index][column])
        index = 0xAD if 'Update' in name else 1
        target = binary.pointer(binary.ids[id] + index * 8)
        assert target == expected[column], (name, hex(target), hex(expected[column]))
        records.append(dict(name=name, id=id, table=table_index, slot=hex(index), target_rva=hex(target)))

    target = binary.ids[41557 if ae else 40551]
    for name, id, offset in [('NPC landing', 37507 if ae else 36507, 0x9BB if ae else 0x89D),
                              ('Player landing', 42350 if ae else 41271, 0x179 if ae else 0x241)]:
        rva = binary.ids[id] + offset
        instruction = binary.bytes(rva, 5)
        assert instruction[0] == 0xE8
        assert rva + 5 + struct.unpack('<i', instruction[1:])[0] == target
        records.append(dict(name=name, id=id, offset=hex(offset), rva=hex(rva),
                            bytes=instruction.hex(' '), target_rva=hex(target)))

    id, offset = (37710, 0x54) if ae else (36700, 0x55)
    rva = binary.ids[id] + offset
    expected = bytes.fromhex('0f 85 d1 01 00 00' if ae else '0f 85 d6 01 00 00')
    assert binary.bytes(rva, 6) == expected
    assert binary.bytes(rva - 5, 5) == bytes.fromhex('c1 e8 0d a8 01')
    records.append(dict(name='Stagger state check', id=id, offset=hex(offset), rva=hex(rva), bytes=expected.hex(' ')))
    return dict(runtime=runtime, checks=records)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Audit the nine hook/patch locations in an unpacked Skyrim executable.')
    parser.add_argument('--exe', required=True)
    parser.add_argument('--library', required=True)
    parser.add_argument('--commonlib', required=True)
    parser.add_argument('--runtime', required=True, choices=['1597', '1170'])
    parser.add_argument('--output')
    args = parser.parse_args()
    result = json.dumps(audit(args.exe, args.library, args.commonlib, args.runtime), indent=2)
    if args.output:
        Path(args.output).write_text(result + '\n', encoding='utf-8')
    print(result)
