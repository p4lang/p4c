<!--!
\page getting_started Getting Started                                    
-->
<!--!
\internal
-->
# P4C
<!--!
\endinternal
-->
<!--!
[TOC]
-->
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-debian.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-debian.yml)
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-fedora.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-fedora.yml)
[![Main Build](https://github.com/p4lang/p4c/actions/workflows/ci-test-mac.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-test-mac.yml)
[![Bazel Build](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-bazel.yml)
[![Validation](https://github.com/p4lang/p4c/actions/workflows/ci-validation-nightly.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-validation-nightly.yml)
[![Docker Container](https://github.com/p4lang/p4c/actions/workflows/ci-container-image.yml/badge.svg)](https://github.com/p4lang/p4c/actions/workflows/ci-container-image.yml)

<!--!
\internal
-->
* [Sample Backends in P4C](#sample-backends-in-p4c)
* [Getting started](#getting-started)
   * [Installing packaged versions of P4C](#installing-packaged-versions-of-p4c)
   * [Installing P4C from source](#installing-p4c-from-source)
* [Dependencies](#dependencies)
   * [Ubuntu dependencies](#ubuntu-dependencies)
   * [Fedora dependencies](#fedora-dependencies)
   * [macOS dependencies](#macos-dependencies)
   * [Garbage collector](#garbage-collector)
   * [Crash dumps](#crash-dumps)
* [Development tools](#development-tools)
   * [Git setup](#git-setup)
* [Docker](#docker)
* [Bazel](#bazel)
* [Build system](#build-system)
   * [Defining new CMake targets](#defining-new-cmake-targets)
* [Known issues](#known-issues)
   * [Frontend](#frontend)
   * [Backends](#backends)
* [How to Contribute](#how-to-contribute)
* [P4 Compiler Onboarding](#p4-compiler-onboarding)
* [Contact](#contact)
<!--!
\endinternal
-->
P4C is a reference compiler for the P4 programming language.
It supports both P4-14 and P4-16; you can find more information about P4
[here](http://p4.org) and the specifications for both versions of the language
[here](https://p4.org/specs).
One fact attesting to the level of quality and completeness of P4C's
code is that its front-end code, mid-end code, and P4C-graphs back end
are used as the basis for at least one commercially supported P4
compiler.

P4C is modular; it provides a standard frontend and midend which can be combined
with a target-specific backend to create a complete P4 compiler. The goal is to
make adding new backends easy.

<!--!
\include{doc} "../docs/doxygen/01_overview.md"
-->
## Sample Backends in P4C
P4C includes seven sample backends, catering to different target architectures and use cases:
* p4c-bm2-ss: can be used to target the P4 `simple_switch` written using
  the [BMv2 behavioral model](https://github.com/p4lang/behavioral-model),
* p4c-dpdk: can be used to target the [DPDK software switch (SWX) pipeline](https://doc.dpdk.org/guides/rel_notes/release_20_11.html),
* p4c-ebpf: can be used to generate C code which can be compiled to [eBPF](https://en.wikipedia.org/wiki/Berkeley_Packet_Filter)
  and then loaded in the Linux kernel. The eBPF backend currently implements three architecture models:
  [ebpf_model.p4 for packet filtering, xdp_model.p4 for XDP](./backends/ebpf/README.md) and
  [the fully-featured PSA (Portable Switch Architecture) model](./backends/ebpf/psa/README.md).
* p4test: a source-to-source P4 translator which can be used for
  testing, learning compiler internals and debugging,
* p4c-graphs: can be used to generate visual representations of a P4 program;
  for now it only supports generating graphs of top-level control flows, and
* p4c-ubpf: can be used to generate eBPF code that runs in user-space.
* p4tools: a platform for P4 test utilities, including a test-case generator for P4 programs.
Sample command lines:

Compile P4_16 or P4_14 source code.  If your program successfully
compiles, the command will create files with the same base name as the
P4 program you supplied, and the following suffixes instead of the
`.p4`:

+ a file with suffix `.p4i`, which is the output from running the
  preprocessor on your P4 program.
+ a file with suffix `.json` that is the JSON file format expected by
  BMv2 behavioral model `simple_switch`.

```bash
p4c --target bmv2 --arch v1model my-p4-16-prog.p4
p4c --target bmv2 --arch v1model --std p4-14 my-p4-14-prog.p4
```

By adding the option `--p4runtime-files <filename>.txt` as shown in
the example commands below, P4C will also create a file
`<filename>.txt`.  This is a text format "P4Info" file, containing a
description of the tables and other objects in your P4 program that
have an auto-generated control plane API.

```
p4c --target bmv2 --arch v1model --p4runtime-files my-p4-16-prog.p4info.txt my-p4-16-prog.p4
p4c --target bmv2 --arch v1model --p4runtime-files my-p4-14-prog.p4info.txt --std p4-14 my-p4-14-prog.p4
```

All of these commands take the `--help` argument to show documentation
of supported command line options.  `p4c --target-help` shows the
supported "target, arch" pairs.

```bash
p4c --help
p4c --target-help
```

Auto-translate P4_14 source to P4_16 source:

```bash
p4test --std p4-14 my-p4-14-prog.p4 --pp auto-translated-p4-16-prog.p4
```

Check syntax of P4_16 or P4_14 source code, without limitations that
might be imposed by any particular compiler back end.  There is no
output for these commands other than error and/or warning messages.

```bash
p4test my-p4-16-prog.p4
p4test --std p4-14 my-p4-14-prog.p4
```

Generate GraphViz ".dot" files for parsers and controls of a P4_16 or
P4_14 source program.

```bash
p4c-graphs my-p4-16-prog.p4
p4c-graphs --std p4-14 my-p4-14-prog.p4
```

Generate PDF of parser instance named "ParserImpl" generated by the
`p4c-graphs` command above (search for graphviz below for its install
instructions):

```bash
dot -Tpdf ParserImpl.dot > ParserImpl.pdf
```

## Getting started

### Installing packaged versions of P4C

P4C has package support for several Ubuntu and Debian distributions.

#### Ubuntu

A P4C package is available in the following repositories for Ubuntu 20.04 and newer.

```bash
source /etc/lsb-release
echo "deb http://download.opensuse.org/repositories/home:/p4lang/xUbuntu_${DISTRIB_RELEASE}/ /" | sudo tee /etc/apt/sources.list.d/home:p4lang.list
curl -fsSL https://download.opensuse.org/repositories/home:p4lang/xUbuntu_${DISTRIB_RELEASE}/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
sudo apt-get update
sudo apt install p4lang-p4c
```

#### Debian

For Debian 11 (Bullseye) it can be installed as follows:

```bash
echo 'deb https://download.opensuse.org/repositories/home:/p4lang/Debian_11/ /' | sudo tee /etc/apt/sources.list.d/home:p4lang.list
curl -fsSL https://download.opensuse.org/repositories/home:p4lang/Debian_11/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/home_p4lang.gpg > /dev/null
sudo apt update
sudo apt install p4lang-p4c
```

If you cannot use a repository to install P4C, you can download the `.deb` file
for your release and install it manually. You need to download a new file each
time you want to upgrade P4C.

1. Go to [p4lang-p4c package page on OpenSUSE Build Service](https://build.opensuse.org/package/show/home:p4lang/p4lang-p4c), click on
"Download package" and choose your operating system version.

2. Install P4C, changing the path below to the path where you downloaded the package.

```bash
sudo dpkg -i /path/to/package.deb
```

### Installing P4C from source
1.  Clone the repository.
    ```
    git clone https://github.com/p4lang/p4c.git
    ```

2.  Install [dependencies](#dependencies). You can find specific instructions
    for Ubuntu 22.04 [here](#ubuntu-dependencies) and for macOS 11
    [here](#macos-dependencies).  You can also look at the
    [CI installation script](https://github.com/p4lang/p4c/blob/main/tools/ci-build.sh).

3.  Build. Building should also take place in a subdirectory named `build`.
    ```
    mkdir -p build
    cmake -B build <optional arguments>
    cmake --build build
    cmake --build build --target check
    ```
    The cmake command takes the following optional arguments to
    further customize the build (see file `CMakeLists.txt` for the full list):
     - `-DCMAKE_BUILD_TYPE=Release|Debug` -- set CMAKE_BUILD_TYPE to
      Release or Debug to build with optimizations or with debug
      symbols to run in gdb. Default is Release.
     - `-DCMAKE_INSTALL_PREFIX=<path>` -- set the directory where
     `cmake --build build --target install` installs the compiler. Defaults to /usr/local.
     - `-DENABLE_BMV2=ON|OFF`. Enable [the bmv2 backend](backends/bmv2/README.md). Default ON.
     - `-DENABLE_EBPF=ON|OFF`. Enable [the ebpf backend](backends/ebpf/README.md). Default ON.
     - `-DENABLE_P4TC=ON|OFF`. Enable [the TC backend](backends/tc/README.md). Default ON.
     - `-DENABLE_UBPF=ON|OFF`. Enable [the ubpf backend](backends/ubpf/README.md). Default ON.
     - `-DENABLE_DPDK=ON|OFF`. Enable [the DPDK backend](backends/dpdk/README.md). Default ON.
     - `-DENABLE_P4C_GRAPHS=ON|OFF`. Enable [the p4c-graphs backend](backends/graphs/README.md). Default ON.
     - `-DENABLE_P4FMT=ON|OFF`. Enable [the p4fmt backend](backends/p4fmt/README.md). Default ON.
     - `-DENABLE_P4TEST=ON|OFF`. Enable [the p4test backend](backends/p4test/README.md). Default ON.
     - `-DENABLE_TEST_TOOLS=ON|OFF`. Enable [the p4tools backend](backends/p4tools/README.md). Default OFF.
     - `-DENABLE_DOCS=ON|OFF`. Build documentation. Default is OFF.
     - `-DENABLE_GC=ON|OFF`. Enable the use of the garbage collection
       library. Default is ON.
     - `-DENABLE_GTESTS=ON|OFF`. Enable building and running GTest unit tests.
       Default is ON.
     - `-DP4C_USE_PREINSTALLED_ABSEIL=ON|OFF`. Try to find a system version of Abseil instead of a fetched one. Default is OFF.
     - `-DP4C_USE_PREINSTALLED_PROTOBUF=ON|OFF`. Try to find a system version of Protobuf instead of a CMake version. Default is OFF.
     - `-DENABLE_ABSEIL_STATIC=ON|OFF`. Enable the use of static abseil libraries. Default is ON. Only has an effect when `P4C_USE_PREINSTALLED_ABSEIL` is enabled.
     - `-DENABLE_PROTOBUF_STATIC=ON|OFF`. Enable the use of static protobuf libraries. Default is ON.
       Only has an effect when `P4C_USE_PREINSTALLED_PROTOBUF` is enabled.
     - `-DENABLE_MULTITHREAD=ON|OFF`. Use multithreading.  Default is
       OFF.
     - `-DBUILD_LINK_WITH_GOLD=ON|OFF`. Use Gold linker for build if available.
     - `-DBUILD_LINK_WITH_LLD=ON|OFF`. Use LLD linker for build if available (overrides `BUILD_LINK_WITH_GOLD`).
     - `-DENABLE_LTO=ON|OFF`. Use Link Time Optimization (LTO).  Default is OFF.
     - `-DENABLE_WERROR=ON|OFF`. Treat warnings as errors.  Default is OFF.
     - `-DCMAKE_UNITY_BUILD=ON|OFF `. Enable [unity builds](https://cmake.org/cmake/help/latest/prop_tgt/UNITY_BUILD.html) for faster compilation.  Default is OFF.

    If adding new targets to this build system, please see
    [instructions](#defining-new-cmake-targets).

4.  (Optional) Install the compiler and the P4 shared headers globally.
    ```
    sudo cmake --build build --target install
    ```
    The compiler driver `p4c` and binaries for each of the backends are
    installed in `/usr/local/bin` by default; the P4 headers are placed in
    `/usr/local/share/p4c`.

5.  You're ready to go! You should be able to compile a P4-16 program for BMV2
    using:
    ```
    p4c -b bmv2-ss-p4org program.p4 -o program.bmv2.json
    ```

If you plan to contribute to P4C, you'll find more useful information
[here](#development-tools).

## Dependencies

P4C is supported on Ubuntu 22.04 and 24.04. macOS, including Apple Silicon (M1), is also supported.

- A C++17 compiler. GCC 9.4 or later or Clang 10.0 or later is required.

- `git` for version control

- CMake 3.16.3 or higher

- Boehm-Weiser garbage-collector C++ library

- GNU Bison and Flex for the parser and lexical analyzer generators.

- Google Protocol Buffers (handled automatically via CMake FetchContent)

- C++ boost library

- Python 3 and uv for scripting and running tests

- Optional: Documentation generation requires Doxygen (1.13.2) and Graphviz (2.38.0 or higher).

Backends may have additional dependencies. The dependencies for the backends
included with `P4C` are documented here:
  * [BMv2](backends/bmv2/README.md)
  * [eBPF](backends/ebpf/README.md)
  * [graphs](backends/graphs/README.md)

