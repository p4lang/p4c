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
- [Limitations](#limitations)
- [Further Reading](#further-reading)
- [Citation](#citation)
- [Contributing](#contributing)
- [License](#license)

RtSmith is a [P4Tools](https://github.com/p4lang/p4c/tree/main/backends/p4tools) module that
generates random control-plane configurations from a compiled P4 program's API description. It is
designed to stress-test control-plane tooling and runtimes that ingest P4Runtime configuration by
producing many valid configuration files and update sequences.

Given a P4 program, architecture, and target, RtSmith emits:

- an initial configuration, and
- an optional update time series of `WriteRequest` operations (insert / modify / delete).

RtSmith currently emits [P4Runtime](https://github.com/p4lang/p4runtime) Protobuf text format.
For `v1model`, generated Protobufs follow the latest
[P4Runtime specification](https://p4lang.github.io/p4runtime/spec/main/P4Runtime-Spec.html).
This repository includes BMv2 and Tofino targets.

RtSmith validates configuration shape (for example, IDs, types, field widths, and message structure), but it does not choose entries to force specific packet behavior (for example, forwarding, drop, or table-hit coverage).

## Installation

RtSmith depends on the P4Tools framework and is automatically installed with P4Tools. Please follow the instructions listed [here](https://github.com/p4lang/p4c/tree/main/backends/p4tools#building) to install RtSmith. The main binary `p4rtsmith` can be found in the `build` folder after a successful installation.

```sh
cmake -B build -DENABLE_TEST_TOOLS=ON -DENABLE_TOOLS_MODULE_RTSMITH=ON
cmake --build build --target p4rtsmith
```

Optional tests:

```sh
cmake --build build --target rtsmith-gtest
ctest --test-dir build --output-on-failure -R rtsmith-gtest
```

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
./build/p4rtsmith --target bmv2 --arch v1model --output-dir out path/to/program.p4
```

Generate and save P4Info:

```sh
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
- `--control-plane P4RUNTIME|BFRUNTIME`: select control-plane API mode.
- `--seed <n>` or `--random-seed`: control RNG behavior.

## Limitations

- Entry generation is random. RtSmith does not attempt to generate specific packet behaviors or test specific control-plane features using more advanced techniques such as SMT solver.
- Output is limited to control-plane artifacts (currently Protobuf text output).

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
