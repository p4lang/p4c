<!--!
\page rtsmith RtSmith
-->
<!--
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.
-->
<!--!
\internal
-->
# RtSmith
<!--!
\endinternal
-->

<!--!
[TOC]
-->

[![Status](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml)

## Table of Contents

- [Installation](#installation)
- [Extensions](#extensions)
- [Usage](#usage)
- [Scope](#scope)
- [Limitations](#limitations)
- [Further Reading](#further-reading)
- [Citation](#citation)
- [Contributing](#contributing)
- [License](#license)

RtSmith is a [P4Tools](https://github.com/p4lang/p4c/tree/main/backends/p4tools) module that
generates random control-plane configurations from a compiled P4 program's API description. It is
designed to stress-test control-plane tooling and runtimes that ingest P4Runtime configuration by
producing random table entries and update sequences.

Given a P4 program, architecture, and target, RtSmith emits:

- an initial configuration, and
- an optional update time series of `WriteRequest` operations (insert / modify / delete).

The BMv2 `v1model` target emits P4Runtime `WriteRequest` messages in Protobuf text
format. The experimental Tofino `tna` target emits BfRuntime messages. The two formats
are different and must be consumed by their corresponding runtime.

## Scope

RtSmith is a stochastic fuzzer for control-plane consumers: it generates varied inputs
to help find crashes, performance problems, and pathological update sequences.
The generator does not send requests to a live switch or measure performance itself.

Entries are random; there is no packet-fate or coverage objective. On programs such as
SAI-P4, random entries may mostly cause drops. RtSmith does not currently solve this
problem or guarantee useful forwarding behavior.

## Installation

RtSmith depends on the P4Tools framework and is automatically installed with P4Tools. Please follow the instructions listed [here](https://github.com/p4lang/p4c/tree/main/backends/p4tools#building) to install RtSmith. The main binary `p4rtsmith` can be found in the `build` folder after a successful installation.

```sh
cmake -B build -DENABLE_TEST_TOOLS=ON -DENABLE_CONTROL_PLANE=ON -DENABLE_TOOLS_MODULE_RTSMITH=ON
cmake --build build --target p4rtsmith
```

Tests (configure with `-DENABLE_GTESTS=ON`):

```sh
cmake --build build --target rtsmith-gtest
ctest --test-dir build --output-on-failure -R rtsmith
```

The BMv2 replay tests require `p4c-bm2-ss`, `simple_switch_grpc`, and the PTF Python
dependencies used by the P4Tools CI job. They apply the initial configuration and every
update to BMv2, then check forwarding. When run with `sudo`, the shared BMv2 runner
creates a network namespace for each test, allowing parallel execution:

```sh
sudo -E env PATH="$PATH" ctest --test-dir build -R rtsmith -j4 --output-on-failure
```

Non-root runs use nanomsg in the host network namespace; use `-j1` for those runs
to avoid races when choosing gRPC and Thrift ports.

## Extensions

RtSmith extensions map architecture/target pairs to target-specific entry generation code.

- `targets/bmv2`: BMv2-specific entry generation.
- `targets/tofino`: Tofino-specific entry generation.

## Usage

Display all options:

```sh
./build/p4rtsmith --help
```

Generate configuration from a P4 program:

```sh
./build/p4rtsmith --target bmv2 --arch v1model --seed 1 --output-dir out path/to/program.p4
```

Generate and save P4Info:

```sh
mkdir -p out
./build/p4rtsmith --target bmv2 --arch v1model --generate-p4info out/p4info.txtpb path/to/program.p4
```

Use a user-provided P4Info:

```sh
./build/p4rtsmith --target bmv2 --arch v1model --user-p4info out/p4info.txtpb --output-dir out path/to/program.p4
```

Useful options:

- `--output-dir <dir>`: write generated config files to a directory.
- `--config-name <name>`: base name for emitted config files.
- `--print-to-stdout`: print generated configurations to stdout.
- `--generate-p4info <file.txtpb>`: emit generated P4Info.
- `--user-p4info <file.txtpb>`: use user-provided P4Info instead of generating one.
- `--toml <config.toml>`: configure fuzzing parameters from TOML.
- `--control-plane P4RUNTIME|BFRUNTIME`: must match the target. BMv2 uses the default
  `P4RUNTIME`; Tofino requires `--target tofino1 --arch tna --control-plane BFRUNTIME`.
- `--seed <n>` or `--random-seed`: control RNG behavior.

Files are named `initial_config.txtpb` and `update_1.txtpb`, `update_2.txtpb`, etc.
With `--config-name trial`, they become `trial_initial_config.txtpb` and
`trial_update_1.txtpb`, etc. Use a fresh output directory for each run so that stale
updates from earlier runs are not replayed. Replay updates in numeric order after the
initial configuration. The C++ API also returns a delay in microseconds for each update;
these delays are not encoded in the text files. Consumers must set the device and election
IDs appropriate for their P4Runtime session.

TOML settings are optional overrides; omitted fields retain their defaults. For BMv2:

| Setting | Default | Meaning |
| --- | --- | --- |
| `maxEntryGenCnt` | 5 | Maximum operations per table per request. |
| `maxAttempts` | 100 | Maximum generation attempts per table per request. |
| `maxTables` | 5 | Maximum eligible tables per request, in P4Info order. |
| `tablesToSkip` | `[]` | Fully qualified P4Info table names to exclude. |
| `thresholdForDeletion` | 30 | Deletion percentage when choosing an existing entry. |
| `maxUpdateCount` | 10 | Upper bound on the randomly chosen number of update requests. |
| `minUpdateTimeInMicroseconds` | 50000 | Minimum delay associated with an update. |
| `maxUpdateTimeInMicroseconds` | 100000 | Maximum delay associated with an update. |

## Limitations

- BMv2 generation covers direct tables with exact, LPM, ternary, range, and optional
  matches. It respects action scope, match encoding, table capacity, and the state of
  entries generated in the current run. It assumes those tables start empty.
- Keyless, constant, preinitialized, and indirect tables are skipped. Action profile
  members/groups, default-action updates, and other extern entities are not generated.
- `p4-constraints`, `@refers_to`, translated types, and target-specific semantic or
  resource constraints are not enforced. Generated requests are not guaranteed to be
  accepted by every consumer. Supply a matching P4Info when using `--user-p4info`.
- The Tofino generator is experimental: it generates initial entries only, does not
  implement the BMv2 TOML controls, and is not validated against a Tofino runtime in CI.
  It needs a suitable user-supplied P4Info when the compiler has no Tofino serializer.
- RtSmith does not synthesize packets or select entries for packet behavior or coverage.
  Output is limited to control-plane artifacts in Protobuf text format.

## Further Reading

RtSmith was originally developed as ControlPlaneSmith as part of the partial evaluation project, Flay.

If you would like to cite this tool please use this citation format:

```bibtex
@inproceedings{ruffy2024incremental,
  author    = {Ruffy, Fabian and Wang, Zhanghan and Antichi, Gianni and Panda, Aurojit and Sivaraman, Anirudh},
  title     = {Incremental Specialization of Network Programs},
  year      = {2024},
  publisher = {Association for Computing Machinery},
  address   = {New York, NY, USA},
  booktitle = {Proceedings of the 23rd ACM Workshop on Hot Topics in Networks},
  pages     = {264--272},
  doi       = {10.1145/3696348.3696870},
  abstract  = {Programmable network devices process packets using limited time and space. Consequently, much effort has been spent making network programs run as efficiently as possible. One promising line of work focuses on specializing the implementation of a network program to a particular---presumed constant---control-plane configuration. However, while some parts of the control plane configurations are constant for long periods of time, others change frequently, and in bursts (e.g., due to routing table updates).
Thus, any approach that specializes a network program with respect to control-plane configurations must be incremental: it should be able to tell quickly whether a new control-plane update will affect the program's implementation and recompile the program only when its implementation must change. We describe several benefits of such an approach, including reducing resource use on line-rate pipelines and improving the memory footprint of packet classification. We explore our ideas with a prototype, Flay, an incremental partial evaluator that optimizes P4 programs by treating control-plane entries as constant. Flay can reduce resources in the implementations of Tofino programs. Flay can also determine in 100s of milliseconds whether a control-plane update will change a program's implementation. We conclude by outlining several avenues for future work.}
}
```

## Contributing

Contributions to RtSmith in any form are welcome! Please follow the guidelines listed [here](https://github.com/p4lang/p4c/blob/main/CONTRIBUTING.md) to contribute.

## License

This project is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/p4lang/p4c/blob/main/backends/p4tools/LICENSE) file for details.
