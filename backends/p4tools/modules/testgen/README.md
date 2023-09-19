[![Status](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-p4tools.yml)

# <img src="https://p4.org/wp-content/uploads/2021/05/Group-81.png" width="40">Testgen

## Table of Contents

- [Installation](#installation)
- [Extensions](#extensions)
- [Usage](#usage)
    - [Coverage](#coverage)
    - [Generating Specific Tests](#generating-specific-tests)
    - [Interacting with Test Frameworks](#interacting-with-test-frameworks)
    - [Detecting P4 Program Flaws](#detecting-p4-program-flaws)
- [Limitations](#limitations)
- [Further Reading](#further-reading)
- [Contributing](#contributing)
- [License](#license)

P4Testgen is an extensible [test oracle](https://en.wikipedia.org/wiki/Test_oracle) that uses symbolic execution to automatically generate input-output tests for P4 programs. P4Testgen is part of the P4Tools and P4C ecosystem.

## Features

- **Exhaustive, automatic input-output test generation**: Given a P4 program and sufficient time, P4Testgen generates tests that cover every reachable statement in the program. Each test consists of an input packet, control-plane configuration, and the expected output packet. Test case generation is configurable using command-line flags and assertions within the P4 code. For example, it is possible to generate tests that use only TCP-IP packets as input.

- **Support for multiple P4 targets**: P4Testgen is designed to be extensible for many P4 targets. It models the complete semantics of the target's packet-processing pipeline, including the P4 language, architectures, externs, and target-specific behaviors. Support for a P4 target is implemented in the form of an extension. The open-source version of P4Testgen supports generating tests for the [v1model.p4](https://github.com/p4lang/behavioral-model/blob/main/docs/simple_switch.md) architecture on [BMv2](https://github.com/p4lang/behavioral-model), the [pna.p4](https://p4.org/p4-spec/docs/pna-working-draft-html-version.html) architecture on [DPDK-SoftNIC](https://github.com/p4lang/p4-dpdk-target), and the [ebpf_model.p4](https://github.com/p4lang/p4c/tree/main/backends/ebpf) architecture on [the Linux kernel](http://vger.kernel.org/lpc_net2018_talks/p4c-xdp-lpc18-paper.pdf).

- **Support for multiple test frameworks**: P4Testgen generates abstract tests that can be serialized into formats suitable for any test framework. P4Testgen extensions can generate tests for STF, PTF, and P4Runtime Protobuf messages.

- **Coverage heuristics**: To limit the number of generated tests, P4Testgen uses coverage heuristics. It employs coverage-guided search to generate tests that cover new nodes in the P4 program, such as statements or tables. P4Testgen also supports custom stop metrics to ensure that test case generation stops when a user-defined goal has been reached.

## Installation

P4Testgen depends on the P4Tools framework and is automatically installed with P4Tools. Please follow the instructions listed [here](https://github.com/p4lang/p4c/tree/main/backends/p4tools#building) to install P4Testgen. The main binary `p4testgen` can be found in the `build` folder after a successful installation.

P4Testgen is available as part of the [official P4C docker image](https://hub.docker.com/r/p4lang/p4c/). On Debian-based systems, it is also possible to install a P4Testgen binary by following [these](https://github.com/p4lang/p4c#installing-packaged-versions-of-p4c) instructions.

## Extensions
P4Testgen extensions are instantiations of a particular combination of P4 architecture and the target that executes the P4 code. For example, the `v1model.p4` architecture can be executed on the behavioral model. P4Testgen extension make use of the core P4Testgen framework to generate tests. Several open-source extensions are available.

### v1model.p4 on BMv2
[targets/bmv2](targets/bmv2)

P4Testgen supports generating [STF](https://github.com/p4lang/p4c/tree/main/tools/stf), [PTF](https://github.com/p4lang/ptf), [Protobuf](https://protobuf.dev/overview/) messages, and Metadata templates for the `v1model` architecture on [BMv2](https://github.com/p4lang/behavioral-model). Almost all externs, including checksums, cloning, recirculation are supported. P4Testgen also supports [P4Constraints](https://github.com/p4lang/p4-constraints) parsing.

### pna.p4 on the DPDK SoftNIC
[targets/pna](targets/pna)

The [DPDK-SoftNIC](https://github.com/p4lang/p4-dpdk-target) is a new target implemented using the [Data Plane Development Kit (DPDK)](https://www.dpdk.org/). The SoftNIC can be programmed using the P4 `pna.p4` architecture.

### ebpf_model.p4 on the eBPF kernel target
[targets/ebpf](targets/ebpf)

The P4Testgen eBPF extension is a proof-of-concept implementation. It supports generating tests for P4 eBPF programs but, as the test framework and extern support is limited, so is the P4Testgen extension.

## Definitions

Useful definitions to keep in mind when using P4Testgen.

#### Paths and Path Constraints
P4Testgen defines a path as a collection of path constraints. A path constraint is a boolean expression composed of constants and symbolic variables at a particular program point. For example, in the common case an if statement will branch into a `true` and a `false` branch. This will produce two new unique paths. P4Testgen only traverses a path if its constraints are feasible. A path constraint is feasible if an SMT solver is able to find a solution for the constraints. A path constraint such as `hdr.eth.eth_type == 0x800 && hdr.eth.eth_type != 0x800` is never feasible.

#### Symbolic Variables
A symbolic variable is a placeholder variable which is frequently part of path constraints. SMT solvers assign values to symbolic variables to satisfy a particular set of path constraints. In P4Testgen, symbolic variables typically correspond to test inputs. For example, the input packet and port are symbolic variables.

#### Taint
P4Testgen uses bit-level taint tracking to keep track of nondeterminism in the P4 program. An expression is considered tainted if the value of its bits (or simply just a sub-expression) is unpredictable. This could happen because of reads from an uninitialized variable, unimplemented checksums, or simply because of random generators. P4Testgen's core framework marks some constructs tainted, for example uninitialized variables, but otherwise leaves it up to the P4Testgen extension whether to mark a particular expression or method call tainted. Similiarly, it is up to the extension how tainted values are resolved. An extension can either ignore them or throw an error.

## Usage
To access the possible options for `p4testgen` use `p4testgen --help`. To generate a test for a particular target and P4 architecture, run the following command:

```bash
./p4testgen --target [TARGET] --arch [ARCH] --max-tests 10 --out-dir [OUT] prog.p4
```
Where `ARCH` specifies the P4 architecture (e.g., v1model.p4) and `TARGET` represents the targeted network device (e.g., BMv2). Choosing `0` as the option for max-tests will cause P4Testgen to generate tests until it has exhausted all possible paths.

### Coverage
P4Testgen is able to track the (source code) coverage of the program it is generating tests for. With each test, P4Testgen can emit the cumulative program coverage it has achieved so far. Test 1 may have covered 2 out 10 P4 nodes, test 2 5 out of 10 P4 nodes, and so on. To enable program coverage, P4Testgen provides the `--track-coverage [NODE_TYPE]` option where `NODE_TYPE` refers to a particular P4 source node. Currently, `STATEMENTS` for P4 program statements and `TABLE_ENTRIES` for constant P4 table entries are supported. Multiple uses of `--track-coverage` are possible.

The option `--stop-metric MAX_NODE_COVERAGE` makes P4Testgen stop once it has hit 100% coverage as determined by `--track-coverage`.

### Generating Specific Tests

P4Testgen supports the use of custom externs to restrict the breadth of possible input-output tests. These externs are `testgen_assume` and `testgen_assert`, which serve two different use cases: Generating restricted tests and finding assertion violations.

#### Restricted Tests
`testgen_assume(expr)` will add `expr` as a necessary path constraints to all subsequent execution. For example, for the following snippet
```p4
state parse_ethernet {
    packet.extract(headers.ethernet);
    testgen_assume(headers.ethernet.ether_type == 0x800);
    transition select(headers.ethernet.ether_type) {
        0x800: parse_ipv4;
        0x86dd: parse_ipv6;
        default: accept;
    }
}
```
only inputs which have `0x800` as Ethertype will be generated. This mode is enable by default and can be disabled with the flag `--disable-assumption-mode`.

#### Finding Assertion Violations
Conversely, `testgen_assert(expr)` can be used to find violations in a particular P4 program. By default, `testgen_assert` behaves like `testgen_assume`. If the flag `--assertion-mode` is enabled, P4Testgen will only generate tests that will cause `expr` to be false and, hence, violate the assertion.
For example, for
```p4
state parse_ethernet {
    packet.extract(headers.ethernet);
    transition select(headers.ethernet.ether_type) {
        0x800: parse_ipv4;
        0x86dd: parse_ipv6;
        default: accept;
    }
}
state parse_ipv4 {
    packet.extract(headers.ipv4);
    testgen_assert(!headers.ipv6.isValid());
    transition select(headers.ethernet.ether_type) {
        0x800: parse_ipv4;
        0x86dd: parse_ipv6;
        default: accept;
    }
}
```
with `--assertion-mode` enabled, P4Testgen will try to generate tests that violated the condition `testgen_assert(!headers.ipv6.isValid());`.

### Interacting with Test Frameworks
Generally, P4Testgen **only** generates tests. It does not invoke test frameworks or run end-to-end tests. However, many of the extensions supply tests that do so. Each extension has their own scripts and CMake implementation for these test scripts. These can be run with `ctest -R testgen-p4c-[extension]`. Concretely, `ctest  -V -R testgen-p4c-bmv2/` will run the v1model BMv2 STF tests.

### Detecting P4 Program Flaws
P4Testgen can also be used to detect flaws in P4 program. P4Testgen supplies a strict mode (enabled with the flag `--strict`), which fails when the interpreter encounters unrecoverable tainted behavior in the program.

Coverage tracking can also be used to identify dead code in the program. If P4Testgen does not achieve 100% coverage within a reasonable amount of time (say 10k tests or an hour of test generation) one can use the  `--print-coverage` to emit the nodes which can not be covered. Often, P4Testgen simply does have control-plane support for the node, but in many cases the code may simply not executable.

## Limitations
P4Testgen only performs functional validation for single inputs. It does not support tests which involve multiple packets as input. It also does not support generating any performance or resource usage tests. Target-specific limitations are documented in the corresponding relevant target-folder or Github issue.

## Further Reading
P4Testgen has been published at SIGCOMM 2023. The paper describing the tool is available [here](https://arxiv.org/abs/2211.15300).

A talk is also available on Youtube: [p4testgen: Automated Test Generation for Real-World P4 Data Planes](https://www.youtube.com/watch?v=yucl3azbh8Q)

If you would like to cite this work please use this citation format:
```latex
@inproceedings{ruffy-sigcom2023,
    author = {Ruffy, Fabian and Liu, Jed and Kotikalapudi, Prathima and Havel, Vojtěch and Tavante, Hanneli and Sherwood, Rob and Dubina, Vladislav and Peschanenko, Volodymyr and Sivaraman, Anirudh and Foster, Nate},
    title = {{P4Testgen}: An Extensible Test Oracle For {P4}},
    booktitle={Proceedings of the ACM SIGCOMM 2023 Conference. 2023},
    year = {2023},
    month = sep,
    publisher = {Association for Computing Machinery},
    abstract = {We present P4Testgen, a test oracle for the P4-16. P4Testgen supports automatic test generation for any P4 target and is designed to be extensible to many P4 targets. It models the complete semantics of the target's packet-processing pipeline including the P4 language, architectures and externs, and target-specific extensions. To handle non-deterministic behaviors and complex externs (e.g., checksums and hash functions), P4Testgen uses taint tracking and concolic execution. It also provides path selection strategies that reduce the number of tests required to achieve full coverage. We have instantiated P4Testgen for the V1model, eBPF, PNA, and Tofino P4 architectures. Each extension required effort commensurate with the complexity of the target. We validated the tests generated by P4Testgen by running them across the entire P4C test suite as well as the programs supplied with the Tofino P4 Studio. Using the tool, we have also confirmed 25 bugs in mature, production toolchains for BMv2 and Tofino.}
}
```

## Contributing

Contributions to P4Testgen in any form are welcome! Please follow the guidelines listed [here](https://github.com/p4lang/p4c/tree/main/docs) to contribute.

## License

This project is licensed under the Apache License 2.0. See the [LICENSE](https://github.com/p4lang/p4c/blob/main/backends/p4tools/LICENSE) file for details.
