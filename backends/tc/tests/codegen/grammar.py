#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Mojatatu Networks
#
# SPDX-License-Identifier: Apache-2.0
"""Check emitted template constants against the iproute2 parser.

Every other oracle in this directory models the runtime's grammar: it decodes a
value with int(token, 0) or with a pattern we wrote down. A form the runtime
cannot read therefore passes. This test asks the real parser instead: it links
the iproute2 tree's tc/p4_types.c, the same code tc uses when it loads a
template, and feeds it every constant the compiler emitted.

Accepted constants are compared with the value AND the mask the runtime
writes: a parser that drops a mask, or one that silently stores only the low
half of a wide value, would otherwise pass on the verdict alone. The expected
pair is a model of the parser's own buffer layout, so a type it does not model
fails the run instead of being skipped.

The parser is built from the tree named by --iproute2 (or $P4TC_IPROUTE2). When
no tree is configured, or the tree is not built, the test skips. The tokens the
parser must refuse are listed below. A tree whose parser accepts them predates
the bit<128>/IPv6-mask grammar fix: the emitted corpus is still checked, the
negative half is only reported, and --expect-fixed turns that report into a
failure for a run that is meant to have the fix.
"""

import argparse
import json
import os
import platform
import shlex
import shutil
import socket
import subprocess
import sys
import tempfile
from pathlib import Path

import register_init
import run

HERE = Path(__file__).resolve().parent

# The compiler emits these; both are compiled here so the parser sees the text
# of the artifact and not a transcription of it.
FIXTURES = {
    'register-init': 'testdata/p4_16_samples/tc-register-init.p4',
    'typed-keys': 'testdata/p4tc_samples/const_typed_keys.p4',
    'action-argument': 'testdata/p4tc_samples/test_ipv6_example.p4',
}

# Values the parser has to refuse. None of them is emitted by the compiler, so
# a parser that accepts one cannot be trusted to judge the emitted text.
REJECTED = [
    ('bit128', '12345', 'decimal where the type requires hexadecimal'),
    ('bit128', '0', 'a bare zero has no prefix'),
    ('bit128', '0100', 'a leading zero is not a hexadecimal prefix'),
    ('bit128', '0ff', 'a leading zero is not a hexadecimal prefix'),
    ('bit128', '0xzz', 'not a number'),
    ('bit32', '0xzz', 'not a number'),
    (
        'ipv6',
        '1:db8:0:0:0:0:0:0/0x00ffffff0000000000000000000000',
        'a mask that is not 32 hexadecimal digits',
    ),
]

LIBRARY_DIRS = (
    '/usr/lib/x86_64-linux-gnu',
    '/usr/lib64',
    '/usr/lib',
    '/lib/x86_64-linux-gnu',
)


def optional_path(value):
    return Path(value) if value else None


def shared_library(name):
    for directory in LIBRARY_DIRS:
        candidate = Path(directory) / name
        if candidate.exists():
            return str(candidate)
    return None


def build_driver(iproute2, output):
    """Compile tc/p4_types.c and the driver into one binary.

    The archive members the parser needs pull in two helpers that live outside
    it: libnetlink for attribute handling and libcap for utils' drop_cap. Both
    are taken from the host, as a shared object when no development symlink is
    installed.
    """
    source = iproute2 / 'tc/p4_types.c'
    if not source.exists():
        return None, f'{source} not found'
    library = iproute2 / 'lib/libutil.a'
    if not library.exists():
        return None, f'{library} not found: run make in {iproute2}'

    command = [
        os.environ.get('CC', 'cc'),
        '-O1',
        '-o',
        str(output),
        str(HERE / 'grammar_driver.c'),
        str(source),
        f'-I{iproute2 / "include"}',
        f'-I{iproute2 / "tc"}',
        f'-I{iproute2 / "include/uapi"}',
        f'-L{iproute2 / "lib"}',
        '-Wl,--start-group',
        '-lutil',
        '-lnetlink',
        '-Wl,--end-group',
    ]
    for name, fallback in (('libmnl', '-lmnl'), ('libcap', '-lcap')):
        command.append(shared_library(f'{name}.so.0') or shared_library(f'{name}.so') or fallback)

    result = subprocess.run(command, capture_output=True)
    if result.returncode:
        return None, result.stderr.decode(errors='replace')
    return output, None


def parse(driver, pairs):
    """Return the parser's verdict for each (ptype, token) pair, in order."""
    payload = ''.join(f'{ptype}\t{token}\n' for ptype, token in pairs)
    result = subprocess.run([str(driver)], input=payload.encode(), capture_output=True)
    if result.returncode:
        raise AssertionError(result.stderr.decode(errors='replace'))
    verdicts = result.stdout.decode().splitlines()
    if len(verdicts) != len(pairs):
        raise AssertionError(f'parser answered {len(verdicts)} of {len(pairs)} tokens')
    return verdicts, result.stderr.decode(errors='replace')


def decode(verdict):
    fields = verdict.split('\t')
    if fields[0] == 'reject':
        return None
    if fields[0] == 'error':
        raise AssertionError(verdict)
    return bytes.fromhex(fields[1]), bytes.fromhex(fields[2])


def wide_bytes(value):
    """bit<128> is spliced as two 64-bit halves, so its buffer is big-endian."""
    return value.to_bytes(16, 'big')


# The parser stores a value in a C container and the driver prints as many
# bytes as the type occupies in a key. The container can be wider than the key
# (a bit<24> is written through a __u32), so the expected bytes are the
# container's memory image truncated to the key's width.
CONTAINER_BITS = (8, 16, 32, 64, 128)


def container_bits(bits):
    for size in CONTAINER_BITS:
        if bits <= size:
            return size
    raise AssertionError(f'no container for a {bits}-bit type')


def key_bytes(bits):
    return (bits + 7) // 8


def prefix_mask(bits, prefix):
    """A mask with the top @p prefix bits set, big-endian."""
    if not 0 <= prefix <= bits:
        raise AssertionError(f'a /{prefix} mask does not fit in {bits} bits')
    if prefix == 0:
        return bytes(bits // 8)
    return (((1 << prefix) - 1) << (bits - prefix)).to_bytes(bits // 8, 'big')


def mac_bytes(value_text):
    """A macaddr value in the layout parse_p4t_mac_val() leaves behind.

    A colon-separated address is copied into the buffer as-is. A numeric
    address is parsed as a big-endian 64-bit word and then shifted on a
    little-endian host, so its bytes are the byte-swapped word shifted right
    by 16.
    """
    if ':' in value_text or '-' in value_text:
        raw = bytes.fromhex(value_text.replace(':', '').replace('-', ''))
        if len(raw) != 6:
            raise AssertionError(f'{value_text!r} is not a 6-byte address')
        return raw
    value = int(value_text, 0)
    swapped = int.from_bytes(value.to_bytes(8, 'big'), 'little') >> 16
    return (swapped & ((1 << 64) - 1)).to_bytes(8, 'little')[:6]


def mac_mask(mask_text):
    """A macaddr mask in the same layout: the low 16 bits are dropped."""
    if not mask_text:
        # max_mask is stored in host order, which on a little-endian host
        # leaves the word's big-endian bytes in the buffer.
        return (0xFFFFFFFFFFFF0000).to_bytes(8, 'big')[:6]
    value = int(mask_text, 0)
    swapped = int.from_bytes(value.to_bytes(8, 'big'), 'little') >> 16
    return (swapped & ((1 << 64) - 1)).to_bytes(8, 'little')[:6]


def expected_pair(ptype, token):
    """The value and mask bytes the runtime's parser writes for @p token.

    The model follows tc/p4_types.c: bit and int types are stored in host
    (little-endian) order, be types, bit<128>, IPv4 and IPv6 are big-endian,
    and only a type flagged HAS_MASK writes a mask. A type this function does
    not model raises, so a new type in the corpus is a reported failure and
    never a silent pass.
    """
    value_text, _, mask_text = token.partition('/')

    if ptype.startswith('bit'):
        bits = int(ptype[3:])
        if bits >= 128:
            # parse_p4t_u128_val() splices two big-endian halves and never
            # writes the mask, because u128_typ has no HAS_MASK flag.
            return wide_bytes(int(value_text, 0)), bytes(16)
        size = container_bits(bits)
        width = key_bytes(bits)
        value = int(value_text, 0)
        mask = int(mask_text, 0) if mask_text else (1 << bits) - 1
        return (
            (value & ((1 << size) - 1)).to_bytes(size // 8, 'little')[:width],
            (mask & ((1 << size) - 1)).to_bytes(size // 8, 'little')[:width],
        )

    if ptype.startswith('int'):
        bits = int(ptype[3:])
        if bits > 64:
            raise AssertionError(f'{ptype} has no parse function')
        size = container_bits(bits)
        value = int(value_text, 0) & ((1 << size) - 1)
        return value.to_bytes(size // 8, 'little')[: key_bytes(bits)], bytes(key_bytes(bits))

    if ptype.startswith('be'):
        bits = int(ptype[2:])
        size = container_bits(bits)
        value = int(value_text, 0)
        mask = int(mask_text, 0) if mask_text else (1 << bits) - 1
        return (
            (value & ((1 << size) - 1)).to_bytes(size // 8, 'big')[: key_bytes(bits)],
            (mask & ((1 << size) - 1)).to_bytes(size // 8, 'big')[: key_bytes(bits)],
        )

    if ptype == 'ipv4':
        if '.' in value_text:
            value = socket.inet_pton(socket.AF_INET, value_text)
        else:
            value = int(value_text, 0).to_bytes(4, 'big')
        if mask_text[:2].lower() == '0x':
            mask = int(mask_text, 16).to_bytes(4, 'big')
        elif mask_text:
            mask = prefix_mask(32, int(mask_text, 0))
        elif '.' in value_text:
            mask = b'\xff' * 4
        else:
            # The numeric form returns before the prefix parser sets a mask.
            mask = bytes(4)
        return value, mask

    if ptype == 'ipv6':
        value = socket.inet_pton(socket.AF_INET6, value_text)
        if mask_text[:2].lower() == '0x':
            if len(mask_text) != 34:
                raise AssertionError(f'{mask_text!r} is not 32 hexadecimal digits')
            mask = bytes.fromhex(mask_text[2:])
        elif mask_text:
            mask = prefix_mask(128, int(mask_text, 0))
        else:
            mask = b'\xff' * 16
        return value, mask

    if ptype == 'macaddr':
        return mac_bytes(value_text), mac_mask(mask_text)

    if ptype == 'bool':
        if token not in ('true', 'false'):
            raise AssertionError(f'{token!r} is not a boolean')
        return bytes([1 if token == 'true' else 0]), bytes(1)

    if ptype == 'dev' and not value_text.startswith('0x') and value_text.isdigit():
        return int(value_text, 0).to_bytes(4, 'little'), bytes(4)

    raise AssertionError(f'no model for the {ptype} type')


def register_defaults(directory):
    """Every extern control-path default, with the type the artifact declares."""
    metadata = json.loads((directory / 'case.json').read_text())
    if not metadata.get('externs'):
        return []
    spellings = register_init.emitted(directory)[1]
    cases = []
    for instance in metadata['externs'][0]['instances']:
        for param in instance['params']:
            if param['attr'] == 'tc_key':
                continue
            token = spellings[instance['inst_name']][param['name']]
            if token is None:
                continue
            label = f'{instance["inst_name"]}.{param["name"]}'
            cases.append((param['type'], token, label))
    return cases


def typed_key_entries(directory):
    """Every key value and mask in a table entry, typed from the introspection."""
    metadata = json.loads((directory / 'case.json').read_text())
    text = (directory / 'case.template').read_text().replace('\\\n', ' ')
    cases = []
    for table in metadata['tables']:
        keys = {key['name']: key for key in table['keyfields']}
        for line in text.splitlines():
            tokens = shlex.split(line)
            if not tokens or 'entry' not in tokens:
                continue
            if not any(token.startswith(f'table/') for token in tokens):
                continue
            position = tokens.index('entry') + 1
            while position + 1 < len(tokens) and tokens[position] in keys:
                name = tokens[position]
                token = tokens[position + 1]
                cases.append((keys[name]['type'], token, name))
                position += 2
    return cases


def action_arguments(directory):
    """Every constant action argument, typed from the introspection."""
    metadata = json.loads((directory / 'case.json').read_text())
    text = (directory / 'case.template').read_text().replace('\\\n', ' ')
    pipeline = metadata.get('pipeline_name', '')
    signatures = {}
    for table in metadata.get('tables', []):
        for action in table['actions']:
            name = f'{pipeline}/{action["name"]}'
            signatures[name] = {param['name']: param for param in action['params']}
    cases = []
    for line in text.splitlines():
        tokens = shlex.split(line)
        if 'action' not in tokens:
            continue
        position = tokens.index('action') + 1
        if position >= len(tokens):
            continue
        name = tokens[position]
        if name not in signatures:
            continue
        position += 1
        while position + 2 < len(tokens) and tokens[position] == 'param':
            argname, token = tokens[position + 1], tokens[position + 2]
            param = signatures[name].get(argname)
            if param is None:
                break
            cases.append((param['type'], token, f'{name}.{argname}'))
            position += 3
    return cases


def collect(compiler, root):
    """Compile the fixtures and gather the constants they emitted."""
    directories = []
    for name, source in FIXTURES.items():
        directory = root / name
        directory.mkdir(exist_ok=True)
        shutil.copyfile(run.ROOT / source, directory / 'case.p4')
        run.execute(
            [compiler, '-I', run.ROOT / 'p4include', directory / 'case.p4', '-o', directory],
            directory,
            'p4',
        )
        directories.append(directory)

    # The 128-bit Register default only exists in the source-level variant that
    # widens the Register, so take it from the one place that defines it.
    variants = root / 'variants'
    variants.mkdir(exist_ok=True)
    register_init.variants(compiler, variants)
    for name in ('wide-128', 'wide-128-leading-zero'):
        directories.append(variants / name)

    cases = []
    for directory in directories:
        cases.extend(register_defaults(directory))
        cases.extend(typed_key_entries(directory))
        cases.extend(action_arguments(directory))
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument(
        '--iproute2',
        type=optional_path,
        default=optional_path(os.environ.get('P4TC_IPROUTE2')),
        help='a built iproute2 tree; the parser is compiled from its tc/p4_types.c',
    )
    parser.add_argument(
        '--expect-fixed',
        action='store_true',
        help='fail instead of skipping when the parser predates the grammar fix',
    )
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()

    if platform.system() != 'Linux' or platform.machine() != 'x86_64':
        print('SKIP: the parser is only exercised on Linux x86-64')
        return 77
    if args.iproute2 is None:
        print('SKIP: no iproute2 tree given (--iproute2 or P4TC_IPROUTE2)')
        return 77
    iproute2 = args.iproute2.resolve()
    if not iproute2.is_dir():
        print(f'SKIP: {iproute2} is not a directory')
        return 77

    root = args.output
    temporary = None
    if root is None:
        temporary = tempfile.TemporaryDirectory(prefix='p4tc-grammar-')
        root = Path(temporary.name)
    root.mkdir(parents=True, exist_ok=True)

    driver, error = build_driver(iproute2, root / 'grammar-driver')
    if driver is None:
        print(f'SKIP: cannot build the parser from {iproute2}\n{error}')
        return 77
    print(f'parser: {iproute2}/tc/p4_types.c')

    cases = collect(args.compiler.resolve(), root)
    if not cases:
        print('FAIL: no emitted constant was collected')
        return 1

    verdicts, diagnostics = parse(driver, [(ptype, token) for ptype, token, _ in cases])
    failures = []
    records = []
    for (ptype, token, label), verdict in zip(cases, verdicts):
        decoded = decode(verdict)
        record = {'label': label, 'type': ptype, 'token': token}
        if decoded is None:
            failures.append(f'{label}: the parser rejects {ptype} value {token!r}')
            record.update(
                {'value': None, 'mask': None, 'expected_value': None, 'expected_mask': None}
            )
            records.append(record)
            continue
        try:
            expected = expected_pair(ptype, token)
        except AssertionError as error:
            failures.append(f'{label}: {ptype} value {token!r} has no expectation: {error}')
            record.update(
                {
                    'value': decoded[0].hex(),
                    'mask': decoded[1].hex(),
                    'expected_value': None,
                    'expected_mask': None,
                }
            )
            records.append(record)
            continue
        if decoded != expected:
            failures.append(
                f'{label}: {token!r} decodes to value {decoded[0].hex()} mask {decoded[1].hex()}, '
                f'expected value {expected[0].hex()} mask {expected[1].hex()}'
            )
        record.update(
            {
                'value': decoded[0].hex(),
                'mask': decoded[1].hex(),
                'expected_value': expected[0].hex(),
                'expected_mask': expected[1].hex(),
            }
        )
        records.append(record)

    # The negative half: the parser has to refuse forms the compiler never
    # emits, or a green run says nothing about the ones it does.
    rejected_verdicts, _ = parse(driver, [(ptype, token) for ptype, token, _ in REJECTED])
    accepted = [
        f'{ptype} {token!r}: the parser accepts it ({reason})'
        for (ptype, token, reason), verdict in zip(REJECTED, rejected_verdicts)
        if decode(verdict) is not None
    ]

    record = {
        'iproute2': str(iproute2),
        'parser': str(driver),
        'checked': len(cases),
        'types': sorted({ptype for ptype, _, _ in cases}),
        'cases': records,
        'strict': not accepted,
        'failures': failures,
        'unexpected_accepts': accepted,
        'diagnostics': diagnostics,
    }
    (root / 'result.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))

    if failures:
        return 1
    if accepted:
        # The corpus half of this test still stands: every constant the
        # compiler wrote was read by the runtime's own parser and decoded to
        # the expected bytes. What a lenient parser cannot show is that it
        # would have rejected a malformed one, so the negative half is
        # reported and only fails when the caller says the fix is in the tree.
        print(
            'NOTE: this iproute2 predates the bit<128>/IPv6-mask grammar fix, '
            'so the negative half is informational here'
        )
        return 1 if args.expect_fixed else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
