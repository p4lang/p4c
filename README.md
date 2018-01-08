[![Build Status](https://travis-ci.org/p4lang/p4c.svg?branch=master)](https://travis-ci.org/p4lang/p4c)

# p4c

p4c is a new, alpha-quality reference compiler for the P4 programming language.
It supports both P4-14 and P4-16; you can find more information about P4
[here](http://p4.org) and the specifications for both versions of the language
[here](http://p4lang.github.io/p4-spec/).

p4c is modular; it provides a standard frontend and midend which can be combined
with a target-specific backend to create a complete P4 compiler. The goal is to
make adding new backends easy.

The code contains four sample backends:
* p4c-bm2-ss: can be used to target the P4 `simple_switch` written using
  the BMv2 behavioral model https://github.com/p4lang/behavioral-model
* p4c-ebpf: can be used to generate C code which can be compiled to EBPF
  https://en.wikipedia.org/wiki/Berkeley_Packet_Filter and then loaded
  in the Linux kernel for packet filtering
* p4test: a source-to-source P4 translator which can be used for
  testing, learning compiler internals and debugging.
* p4c-graphs: can be used to generate visual representations of a P4 program;
  for now it only supports generating graphs of top-level control flows.

# Getting started

1.  Clone the repository. It includes submodules, so be sure to use
    `--recursive` to pull them in:
    ```
    git clone --recursive https://github.com/p4lang/p4c.git
    ```
    If you forgot `--recursive`, you can update the submodules at any time using:
    ```
    git submodule update --init --recursive
    ```

2.  Install [dependencies](#dependencies). You can find specific instructions
    for Ubuntu 16.04 [here](#ubuntu-dependencies) and for macOS 10.12
    [here](#macos-dependencies).

3.  Build. Building should also take place in a subdirectory named `build`.
    ```
    mkdir build
    cd build
    cmake .. [-DCMAKE_BUILD_TYPE=RELEASE|DEBUG] [-DCMAKE_INSTALL_PREFIX=<path>] [-DENABLE_DOCS=ON (default off)] [-DENABLE_P4RUNTIME_TO_PD=OFF (default on)] [-DENABLE_PROTOBUF_STATIC=OFF (default on)]
    make -j4
    make -j4 check
    ```
    If adding new targets to this build system, please see [instructions](#defining-new-cmake-targets).

4.  (Optional) Install the compiler and the P4 shared headers globally.
    ```
    sudo make install
    ```
    The compiler driver `p4c` and binaries for each of the backends are
    installed in `/usr/local/bin` by default; the P4 headers are placed in
    `/usr/local/share/p4c`.

5.  You're ready to go! You should be able to compile a P4-16 program for BMV2
    using:
    ```
    p4c -b bmv2-ss-p4org program.p4 -o program.bmv2.json
    ```

If you plan to contribute to p4c, you'll find more useful information
[here](#development-tools).

# Dependencies

Ubuntu 16.04 is the officially supported platform for p4c. There's also
unofficial support for macOS 10.12. Other platforms are untested; you can try to
use them, but YMMV.

- A C++11 compiler. GCC 4.9 or later or Clang 3.3 or later is required.

- `git` for version control

- GNU autotools for the build process

- CMake 3.0.2 or higher

- Boehm-Weiser garbage-collector C++ library

- GNU Bison and Flex for the parser and lexical analyzer generators.

- Google Protocol Buffers 3.0 or higher for control plane API generation

- GNU multiple precision library GMP

- C++ boost library (minimally used)

- Python 2.7 for scripting and running tests

- Optional: Documentation generation (enabled when configuring with
  --enable-doxygen-doc) requires Doxygen (1.8.10 or higher) and Graphviz
  (2.38.0 or higher).

Backends may have additional dependencies. The dependencies for the backends
included with `p4c` are documented here:
  * [BMv2](backends/bmv2/README.md)
  * [eBPF](backends/ebpf/README.md)
  * [graphs](backends/graphs/README.md)

## Ubuntu dependencies

Most dependencies can be installed using `apt-get install`:

`sudo apt-get install g++ git automake libtool libgc-dev bison flex libfl-dev libgmp-dev libboost-dev libboost-iostreams-dev libboost-graph-dev pkg-config python python-scapy python-ipaddr tcpdump cmake`

For documentation building:
`sudo apt-get install -y doxygen graphviz texlive-full`

An exception is Google Protocol Buffers; `p4c` depends on version 3.0 or higher,
which is not available until Ubuntu 16.10. For earlier releases of Ubuntu,
you'll need to install from source. You can find instructions
[here](https://github.com/google/protobuf/blob/master/src/README.md). **We
recommend that you use version
[3.2.0](https://github.com/google/protobuf/releases/tag/v3.2.0)**. Earlier
versions in the 3 series may not be supported by other p4lang projects, such as
[p4lang/PI](https://github.com/p4lang/PI). More recent versions may work as
well, but all our CI testing is done with version 3.2.0. After cloning protobuf
and before you build, check-out version 3.2.0:

`git checkout v3.2.0`

Please note that while all protobuf versions newer than 3.0 should work for
`p4c` itself, you may run into trouble with some extensions and other p4lang
projects unless you install version 3.2.0, so you may want to install from
source even on newer releases of Ubuntu.

## macOS dependencies

Installing on macOS:

- Enable XCode's command-line tools:
  ```
  xcode-select --install
  ```

- Install Homebrew:
  ```
  /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
  ```
  Be sure to add `/usr/local/bin/` to your `$PATH`.

- Install dependencies using Homebrew:
  ```
  brew install autoconf automake libtool bdw-gc boost bison pkg-config
  ```

  Install GMP built in C++11 mode:
  ```
  brew install gmp --c++11
  ```

  By default, Homebrew doesn't link programs into `/usr/local/bin` if
  they would conflict with a version provided by the base system. This
  includes Bison, since an older version ships with macOS. `make
  check` depends on the newer Bison we just installed from Homebrew
  (see [#83](http://github.com/p4lang/p4c/issues/83)), so you'll want
  to add it to your `$PATH` one way or another. One simple way to do
  that is to request that Homebrew link it into `/usr/local/bin`:
  ```
  brew link --force bison
  ```

  Optional documentation building tools:
  ```
  brew install doxygen graphviz
  ```
  Homebrew offers a `protobuf` formula. It installs version 3.2, which should
  work for p4c itself but may cause problems with some extensions. It's
  preferable to install Protocol Buffers 3.0 from source using the instructions
  [here](https://github.com/google/protobuf/blob/master/src/README.md). Check
  out the newest tag in the 3.0 series (`v3.0.2` as of this writing) before you
  build.

# Development tools

There is a variety of design and development documentation [here](docs/README.md).

We recommend using `clang++` with no optimizations for speeding up
compilation and simplifying debugging.

We recommend installing a new version of [gdb](http://ftp.gnu.org/gnu/gdb).,
because older gdb versions do not always handle C++11 correctly.

We recommend exuberant ctags for navigating source code in Emacs and vi.  `sudo
apt-get install exuberant-ctags.` The Makefile targets `make ctags` and `make
etags` generate tags for vi and Emacs respectively.  (Make sure that you are
using the correct version of ctags; there are several competing programs with
the same name in existence.)

To enable building code documentation, please run `cmake
.. -DENABLE_DOCS=ON`.  This enables the `make docs` rule to generate
documentation. The HTML output is available in
`build/doxygen-out/html/index.html`.

# Docker

A Dockerfile is included. You can generate an image which contains a copy of p4c
in `/p4c/build` by running:

```
docker build -t p4c .
```

On some platforms Docker limits the memory usage of any container, even
containers used during the `docker build` process. On macOS in particular the
default is 2GB, which is not enough to build p4c. Increase the memory limit to
at least 4GB via Docker preferences or you are likely to see "internal compiler
errors" from gcc which are caused by low memory.

# Build system

The build system is based on cmake.  This section describes how it can be customized.

## Defining new CMake targets

When building a new backend target, add it into the development tree in the
extensions subdirectory. The cmake-based build system will automatically include
it if it contains a CMakeLists.txt file.

For a new backend, the cmake file should contain the following rules:

### IR definition files

Backend specific IR definition files should be added to the global list
of IR_DEF_FILES as they are processed together with the core IR files.
Use the following rule:

```
set (IR_DEF_FILES ${IR_DEF_FILES} ${MY_IR_DEF_FILES} PARENT_SCOPE)
```
where `MY_IR_DEF_FILES` is a list of file names with absolute path
(for example, use `${CMAKE_CURRENT_SOURCE_DIR}`).

If in addition you have additional supporting source files, they
should be added to the frontend sources, as follows:

```
set(EXTENSION_FRONTEND_SOURCES ${EXTENSION_FRONTEND_SOURCES} ${MY_IR_SRCS} PARENT_SCOPE)
```
Again, `MY_IR_SRCS` is a list of file names with absolute path.

### Source files

Sources (.cpp and .h) should be added to the cpplint target using the following rule:

```
add_cpplint_files (${CMAKE_CURRENT_SOURCE_DIR} "${MY_SOURCES_AND_HEADERS}")
```

where `mybackend` is the name of the directory you added under extensions.
The p4c CMakeLists.txt will use that name to figure the full path of the files to lint.

### Target

Define a target for your executable. The target should link against
the core `P4C_LIBRARIES` and `P4C_LIB_DEPS`.  `P4C_LIB_DEPS` are
package dependencies. If you need additional libraries for your
project, add them to `P4C_LIB_DEPS`.

In addition, your target should depend on the `genIR` target, since
you need all the IR generation to happen before you start compiling
your backend. If you chose to have your backend as a library (seem the
backends/bmv2 example), the library should depend on `genIR`, and
there is no longer necessary for your executable to depend on it.

```
add_executable(p4c-mybackend ${MY_SOURCES})
target_link_libraries (p4c-mybackend ${P4C_LIBRARIES} ${P4C_LIB_DEPS})
add_dependencies(p4c-mybackend genIR)
```

### Tests

We implemented support equivalent to the automake `make check` rules.
All tests should be included in `make check` and in addition, we support
`make check-*` rules. To enable this support, add the following rules:

```
set(MY_DRIVER <driver or compiler executable>)

set (MY_TEST_SUITES
  ${P4C_SOURCE_DIR}/testdata/p4_16_samples/*.p4
  ${P4C_SOURCE_DIR}/testdata/p4_16_errors/*.p4
  )
set (MY_XFAIL_TESTS
  testdata/p4_16_errors/this_test_fails.p4
 )
p4c_add_tests("mybackend" ${MY_DRIVER} "${MY_TEST_SUITES}" "${MY_XFAIL_TESTS}")
```

In addition, you can add individual tests to a suite using the following macro:
```
set(isXFail FALSE)
set(SWITCH_P4 testdata/p4_14_samples/switch_20160512/switch.p4)

p4c_add_test_with_args ("mybackend" ${MY_DRIVER} ${isXFail}
  "switch_with_custom_profile" ${SWITCH_P4} "-DCUSTOM_PROFILE")
```

See the documentation for
[`p4c_add_test_with_args`](cmake/P4CUtils.cmake) and
[`p4c_add_tests`](cmake/P4CUtils.cmake) for more information on the
arguments to these macros.

To pass custom arguments to p4c, you can set the environment variable `P4C_ARGS`:
```
make check P4C_ARGS="-Xp4c=MY_CUSTOM_FLAG"
```

### Installation

Define rules to install your backend. Typically you need to install
the binary, the additional architecture headers, and the configuration
file for the p4c driver.

```
install (TARGETS p4c-mybackend
  RUNTIME DESTINATION ${P4C_RUNTIME_OUTPUT_DIRECTORY})
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/p4include
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY})
install (FILES ${CMAKE_CURRENT_SOURCE_DIR}/driver/p4c.mybackend.cfg
  DESTINATION ${P4C_ARTIFACTS_OUTPUT_DIRECTORY}/p4c_src)
```

# Known issues

The P4C compiler is in early development. Issues with the compiler are
tracked on [GitHub](https://github.com/p4lang/p4c/issues). Before
opening a new issue, please check whether a similar issue is already
opened. Opening issues and submitting a pull request with fixes for
those issues is much appreciated.

In addition to the list of issues on Github, there are a number of
currently unsupported features listed below:

## Frontend

### P4_14 features not supported in P4_16

* extern/blackbox attributes -- there is support for carrying them in
the IR, but they are lost if P4_16 code is output.  Backends can
access them from the IR

* Nonstandard extension primitives from P4_14
  * Execute_meter extra arguments
  * Recirculate variants
  * Bypass_egress
  * Sample_ primitives
  * invalidate

* No support for P4_14 parser exceptions.

## Backends

### Bmv2 Backend

* Range "set" not supported in parser transitions
* Tables with multiple apply calls

See also [unsupported P4_16 language features](backends/bmv2/README.md#unsupported-p4_16-language-features).
