#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Mojatatu Networks
#
# SPDX-License-Identifier: Apache-2.0
"""Check Register defaults and execute STF packets with template-seeded helpers.

No host networking, BPF loading or VM operation. Native helper defaults come
from compiler output; expected defaults and packet bytes are authored separately.
"""

import argparse
import json
import platform
import shlex
import struct
import tempfile
from pathlib import Path

import run


def emitted(directory):
    """Return the Register defaults and the text each default was written as.

    The second result keeps the emitted spelling, so a caller can check the form
    a default takes and not only the number it decodes to.
    """
    metadata = json.loads((directory / 'case.json').read_text())
    text = (directory / 'case.template').read_text().replace('\\\n', ' ')
    result, spellings = {}, {}
    for instance in metadata['externs'][0]['instances']:
        name = instance['inst_name']
        (command,) = [line for line in text.splitlines() if '/Register/' + name + ' ' in line]
        tokens = shlex.split(command)
        values, written, raw = {}, {}, bytearray()
        for param in instance['params']:
            if param['attr'] == 'tc_key':
                continue
            offset = next(
                i for i in range(len(tokens) - 1) if tokens[i : i + 2] == ['param', param['name']]
            )
            end = next(
                (
                    i
                    for i in range(offset + 2, len(tokens))
                    if tokens[i] in ('param', 'tc_key', 'tc_data_scalar')
                ),
                len(tokens),
            )
            field = tokens[offset:end]
            spelling = field[field.index('default_value') + 1] if 'default_value' in field else None
            value = int(spelling, 0) if spelling is not None else None
            values[param['name']] = value
            written[param['name']] = spelling
            raw.extend(
                (0 if value is None else value).to_bytes((param['bitwidth'] + 7) // 8, 'little')
            )
        result[name] = values
        spellings[name] = written
        instance['raw_defaults'] = raw
    rows = [
        '#define REGISTER_INSTANCES %d' % len(metadata['externs'][0]['instances']),
        'static const unsigned char register_defaults[REGISTER_INSTANCES + 1][64] = {',
    ]
    for instance in metadata['externs'][0]['instances']:
        rows.append(
            '[%d] = {%s},'
            % (instance['inst_id'], ','.join(str(x) for x in instance['raw_defaults']))
        )
    rows.append('};')
    (directory / 'register_defaults.h').write_text('\n'.join(rows) + '\n')
    return result, spellings


def defaults(directory):
    return emitted(directory)[0]


def wide_form(spelling, value):
    """True when a 128-bit default is written as full-width hexadecimal."""
    return (
        spelling is not None
        and spelling[:2].lower() == '0x'
        and len(spelling) == 34
        and int(spelling, 16) == value
    )


def variants(compiler, root):
    source = (run.ROOT / 'testdata/p4_16_samples/tc-register-init.p4').read_text()
    variants = [
        ('scalar-zero', '(10, 13)', '(10, 0)', 'Ingress.scalar', {'a_value': 0}),
        ('scalar-max', '(10, 13)', '(10, 0xffffffff)', 'Ingress.scalar', {'a_value': 0xFFFFFFFF}),
        (
            'wide-max',
            '0xfedcba9876543210',
            '0xffffffffffffffff',
            'Ingress.wide',
            {'a_value': 0xFFFFFFFFFFFFFFFF},
        ),
        (
            'wide-high-bit',
            '0xfedcba9876543210',
            '0x10000000d',
            'Ingress.wide',
            {'a_value': 0x10000000D},
        ),
        (
            'reordered-struct',
            '{ protocol = 17, aux = 22 }',
            '{ aux = 22, protocol = 17 }',
            'Ingress.pair',
            {'protocol': 17, 'aux': 22},
        ),
    ]
    results = []
    for name, before, after, instance, expected in variants:
        directory = root / name
        directory.mkdir()
        (directory / 'case.p4').write_text(source.replace(before, after))
        run.execute(
            [compiler, '-I', run.ROOT / 'p4include', directory / 'case.p4', '-o', directory],
            directory,
            'p4',
        )
        observed = defaults(directory)[instance]
        results.append(
            {'name': name, 'expected': expected, 'observed': observed, 'pass': observed == expected}
        )
    # A 128-bit default is written at its full width. iproute2 splices the text
    # into two 64-bit halves and shifts a shorter string left rather than
    # right-aligning it, so a value whose top nibbles are zero only reads back
    # correctly when the zeros are written.
    for name, value in [
        ('wide-128', 0xFEDCBA98765432100123456789ABCDEF),
        ('wide-128-leading-zero', 0x0123456789ABCDEF0123456789ABCDEF),
        ('wide-128-one', 1),
        ('wide-128-zero', 0),
    ]:
        directory = root / name
        directory.mkdir()
        wide = source.replace('bit<64>', 'bit<128>').replace('0xfedcba9876543210', hex(value))
        (directory / 'case.p4').write_text(wide)
        run.execute(
            [compiler, '-I', run.ROOT / 'p4include', directory / 'case.p4', '-o', directory],
            directory,
            'p4',
        )
        observed, written = emitted(directory)
        observed = observed['Ingress.wide']
        spelling = written['Ingress.wide']['a_value']
        results.append(
            {
                'name': name,
                'expected': {'a_value': value},
                'observed': observed,
                'spelling': spelling,
                # A decimal form decodes to the same value, so check the text too.
                'pass': observed == {'a_value': value} and wide_form(spelling, value),
            }
        )
    directory = root / 'boolean-diagnostic'
    directory.mkdir()
    boolean = source.replace(
        'Register<bit<32>, bit<32>>(10, 13)', 'Register<bool, bit<32>>(10, true)'
    )
    boolean = boolean.replace(
        'bit<32> value = scalar.read(1);', 'bit<32> value = scalar.read(1) ? 32w1 : 32w0;'
    )
    boolean = boolean.replace('scalar.write(1, value);', 'scalar.write(1, true);')
    (directory / 'case.p4').write_text(boolean)
    result = run.execute(
        [compiler, '-I', run.ROOT / 'p4include', directory / 'case.p4', '-o', directory],
        directory,
        'p4',
        success=False,
    )
    results.append(
        {
            'name': 'boolean-diagnostic',
            'exit': result.returncode,
            'pass': result.returncode != 0
            and b'extern parameter initializer must be an integer constant' in result.stderr,
        }
    )
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if platform.system() != 'Linux' or platform.machine() != 'x86_64':
        return 77
    root = args.output
    temporary = None
    if root is None:
        temporary = tempfile.TemporaryDirectory(prefix='p4tc-register-init-')
        root = Path(temporary.name)
    root.mkdir(parents=True, exist_ok=True)
    observed = {}

    def prepare(directory):
        observed.update(defaults(directory))

    driver = run.build(
        args.compiler.resolve(),
        root / 'case',
        'register-init',
        [],
        prepare=prepare,
        extra_sources=[run.HERE / 'register_init_driver.c'],
    )
    expected = {
        'Ingress.scalar': {'a_value': 13},
        'Ingress.wide': {'a_value': 0xFEDCBA9876543210},
        'Ingress.pair': {'protocol': 17, 'aux': 22},
        'Ingress.zero': {'a_value': 0},
        'Ingress.unspecified': {'a_value': None},
    }
    packets, wanted = [], []
    for line in (run.ROOT / 'testdata/p4_16_samples/tc-register-init.stf').read_text().splitlines():
        tokens = line.split()
        if tokens and tokens[0] in ('packet', 'expect'):
            (packets if tokens[0] == 'packet' else wanted).append(bytes.fromhex(tokens[2]))
    assert len(packets) == len(wanted) == 2
    data = b''.join(struct.pack('=4I', len(packet), 0, 0, 0) + packet for packet in packets)
    output = run.execute([driver], root / 'case', 'native', data=data).stdout
    received, offset = [], 0
    for packet in packets:
        header = struct.unpack_from('=17I', output, offset)
        offset += 68
        length = header[4]
        assert header[0] == 3 and header[1] == 7 and header[2] == 0, header
        received.append(output[offset : offset + length])
        offset += length
    record = {
        'defaults': observed,
        'expected_defaults': expected,
        'defaults_match': observed == expected,
        'packets': [frame.hex() for frame in received],
        'packets_match': received == wanted and offset == len(output),
        'variants': variants(args.compiler.resolve(), root),
    }
    (root / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))
    return (
        0
        if (
            record['defaults_match']
            and record['packets_match']
            and all(row['pass'] for row in record['variants'])
        )
        else 1
    )


if __name__ == '__main__':
    raise SystemExit(main())
