[![Build Status](https://travis-ci.org/p4lang/p4c.svg?branch=master)](https://travis-ci.org/p4lang/p4c)

# p4c

p4c is a new, alpha-quality reference compiler for the P4 programming language.
It supports both P4-14 and P4-16; you can find more information about P4
[here](http://p4.org) and the specifications for both versions of the language
[here](http://p4lang.github.io/p4-spec/).

p4c is modular; it provides a standard frontend and midend which can be combined
with a target-specific backend to create a complete P4 compiler. The goal is to
make adding new backends easy.

The code contains three sample backends:
* p4c-bm2-ss: can be used to target the P4 `simple_switch` written using
  the BMv2 behavioral model https://github.com/p4lang/behavioral-model
* p4c-ebpf: can be used to generate C code which can be compiled to EBPF
  https://en.wikipedia.org/wiki/Berkeley_Packet_Filter and then loaded
  in the Linux kernel for packet filtering
* p4test: a source-to-source P4 translator which can be used for
  testing, learning compiler internals and debugging.

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

3.  Build. By default, building takes place in a subdirectory named `build`.
    ```
    ./bootstrap.sh
    cd build
    make -j4
    make check -j4
    ```

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
    p4c -b bmv2-v1model-p4org program.p4 -o program.bmv2.json
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

- Boehm-Weiser garbage-collector C++ library

- GNU Bison and Flex for the parser and lexical analyzer generators.

- Google Protocol Buffers 3.0 for control plane API generation

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

## Ubuntu dependencies

Most dependencies can be installed using `apt-get install`:

`sudo apt-get install g++ git automake libtool libgc-dev bison flex libgmp-dev libboost-dev libboost-iostreams-dev pkg-config python python-scapy python-ipaddr tcpdump`

For documentation building:
`sudo apt-get install -y doxygen graphviz texlive-full`

An exception is Google Protocol Buffers; `p4c` depends on version 3.0, which is not available until Ubuntu 16.10. For earlier releases of Ubuntu, you'll need to install from source. You can find instructions [here](https://github.com/google/protobuf/blob/master/src/README.md). Check out the newest tag in the 3.0 series (`v3.0.2` as of this writing) before you build.

`git checkout v3.0.2`

Please note that while newer versions should work for `p4c` itself, you may run
into trouble with some extensions unless you install version 3.0, so you may
want to install from source even on newer releases of Ubuntu.

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

To enable building code documentation, please run
`bootstrap.sh --enable-doxygen-doc`. This enables the `make docs` rule to
generate documentation. The HTML output is available in
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
