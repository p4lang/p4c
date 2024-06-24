# P4Smith

<img src="https://p4.org/wp-content/uploads/2021/05/Group-81.png" width="40">
[![Status](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml)

## Table of Contents

- [Installation](#installation)
- [Extensions](#extensions)
- [Usage](#usage)
- [Limitations](#limitations)
- [Further Reading](#further-reading)
- [Contributing](#contributing)
- [License](#license)

P4Smith is an extensible random P4 program generator in the spirit of [CSmith](https://en.wikipedia.org/wiki/Csmith). P4Smith generates random, but valid P4 programs for various P4 targets, for example the [v1model.p4](https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md) architecture on [BMv2](https://github.com/p4lang/behavioral-model) or `tna.p4` running on Tofino 1. P4Smiths generates programs that are valid according to the latest version of the (P4 specification)[https://p4.org/p4-spec/docs/P4-16-working-spec.html] or the restrictions of the specific target.

## Installation

P4Smith depends on the P4Tools framework and is automatically installed with P4Tools. Please follow the instructions listed [here](https://github.com/p4lang/p4c/tree/main/backends/p4tools#building) to install P4Smith. The main binary `p4smith` can be found in the `build` folder after a successful installation.

P4Smith is available as part of the [official P4C docker image](https://hub.docker.com/r/p4lang/p4c/). On Debian-based systems, it is also possible to install a P4Smith binary by following [these](https://github.com/p4lang/p4c#installing-packaged-versions-of-p4c) instructions.

## Extensions
P4Smith extensions are instantiations of a particular combination of P4 architecture and the target that executes the P4 code. For example, the `v1model.p4` architecture can be executed on the behavioral model. P4Smith extension make use of the core P4Smith framework to generate programs. Several open-source extensions are available.

### core.p4 using the test compiler p4test
[targets/generic](targets/generic)

This random program generator generates random packages and tries to produce all valid P4 code according to the latest P4 specification. Programs should be compiled using [p4test](https://github.com/p4lang/p4c/tree/main/backends/p4est).

### v1model.p4 and psa.p4 on BMv2
[targets/bmv2](targets/bmv2)

P4Smith supports generating P4 programs for the `v1model` and `psa` architecture on [BMv2](https://github.com/p4lang/behavioral-model).

### pna.p4 on the DPDK SoftNIC
[targets/fpga](targets/nic)

The [DPDK-SoftNIC](https://github.com/p4lang/p4-dpdk-target) is a new target implemented using the [Data Plane Development Kit (DPDK)](https://www.dpdk.org/). The SoftNIC can be programmed using the P4 `pna.p4` architecture.

### tna.p4 on Tofino 1
[targets/tna](targets/tofino)

P4Smith can also generate programs for the `tna` architecture on Tofino 1. The programs are intended to be compiled on the proprietary Barefoot Tofino compiler.

## Usage
To access the possible options for `p4smith` use `p4smith --help`. To generate a test for a particular target and P4 architecture, run the following command:

```bash
./p4smith --target [TARGET] --arch [ARCH] prog.p4
```
Where `ARCH` specifies the P4 architecture (e.g., v1model.p4) and `TARGET` represents the targeted network device (e.g., BMv2). `prog.p4` is the name of the generated program.

## Further Reading
P4Smith was originally titled Bludgeon and part of the Gauntlet compiler testing framework. Section 4 of the [paper](https://arxiv.org/abs/2006.01074) provides a high-level overview of the tool.


If you would like to cite this tool please use this citation format:
```latex
@inproceedings{ruffy-osdi2020,
author = {Ruffy, Fabian and Wang, Tao and Sivaraman, Anirudh},
title = {Gauntlet: Finding Bugs in Compilers for Programmable Packet Processing},
booktitle = {14th {USENIX} Symposium on Operating Systems Design and Implementation ({OSDI} 20)},
year = {2020},
publisher = {{USENIX} Association},
month = nov,
 abstract = {
  Programmable packet-processing devices such as programmable switches and network interface cards are becoming mainstream. These devices are configured in a domain-specific language such as P4, using a compiler to translate packet-processing programs into instructions for different targets. As networks with programmable devices become widespread, it is critical that these compilers be dependable. This paper considers the problem of finding bugs in compilers for packet processing in the context of P4-16. We introduce domain-specific techniques to induce both abnormal termination of the compiler (crash bugs) and miscompilation (semantic bugs). We apply these techniques to (1) the opensource P4 compiler (P4C) infrastructure, which serves as a common base for different P4 back ends; (2) the P4 back end for the P4 reference software switch; and (3) the P4 back end for the Barefoot Tofino switch. Across the 3 platforms, over 8 months of bug finding, our tool Gauntlet detected 96 new and distinct bugs (62 crash and 34 semantic), which we confirmed with the respective compiler developers. 54 have been fixed (31 crash and 23 semantic); the remaining have been assigned to a developer. Our bug-finding efforts also led to 6 P4 specification changes. We have open sourced Gauntlet at p4gauntlet.github.io and it now runs within P4C’s continuous integration pipeline.}
}

```

## Contributing

Contributions to P4Smith in any form are welcome! Please follow the guidelines listed [here](https://github.com/p4lang/p4c/blob/main/CONTRIBUTING.md) to contribute.

## License

This project is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/p4lang/p4c/blob/main/backends/p4tools/LICENSE) file for details.
