This folder contains the C++ source code for the P4-16 compiler.  This
is a reference implementation of a compiler for the 2016 version of
the P4 programming language, also called P4-16.  The P4-16 draft spec
is available at
http://p4.org/wp-content/uploads/2016/12/P4_16-prerelease-Dec_16.html.
For the P4 programming language see http://p4.org.

The code contains three sample compiler back-ends:
* p4c-bm2-ss: can be used to target the P4 `simple_switch` written using
  the BMv2 behavioral model https://github.com/p4lang/behavioral-model
* p4c-ebpf: can be used to generate C code which can be compiled to EBPF
  https://en.wikipedia.org/wiki/Berkeley_Packet_Filter and then loaded
  in the Linux kernel for packet filtering
* p4test: a source-to-source P4 translator which can be used for
  testing, learning compiler internals and debugging.

Some of these compilers can accept both
[P4-14](http://p4.org/wp-content/uploads/2016/11/p4-spec-latest.pdf)
(i.e., P4 v1.0, v1.1) and
[P4-16](http://p4.org/wp-content/uploads/2016/12/P4_16-prerelease-Dec_16.html)
programs.

The code and documentation are hosted in the following git repository:
https://github.com/p4lang/p4c

The code is currently alpha quality.  Currently there is no way to
install the compiler.

# Dependences

We have tested the compiler on U*X systems (MacOS and Ubuntu).  The
following tools are required to build and run the compiler and tests:

- A C++11 compiler
  E.g., gcc 4.8 or later, or clang++

- `git` for version control

- GNU autotools for the build process

- Boehm-Weiser garbage-collector C++ library

- GNU Bison and Flex (parser and lexical analyzer generators)

- GNU multiple precision library GMP

- C++ boost library (minimally used)

- Python 2.7 for scripting (especially for running tests)

The compiler is modular, and it contains multiple back-ends.  New ones can be added easily.
Each back-end may have additional dependences.  This repository contains the following two
back-ends; please read the following documents for installing more dependences:
  * [BMv2](backends/bmv2/README.md)
  * [eBPF](backends/ebpf/README.md)

## Ubuntu dependences

Most dependences can be installed using `apt-get install`:

`sudo apt-get install g++ git automake libtool libgc-dev bison flex libgmp-dev libboost-dev python2.7 python-scapy python-ipaddr`

## macOS dependences

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

# Development tools

- We recommend installing a new version of gdb, because older gdb versions
  do not always handle C++11 correctly.  See `http://ftp.gnu.org/gnu/gdb`.

- We recommend exuberant ctags for navigating source code in Emacs and
  vi.  `sudo apt-get install exuberant-ctags.` The Makefile targets
  `make ctags` and `make etags` generate tags for vi and Emacs
  respectively.  (Make sure that you are using the correct version of
  ctags; there are several competing programs with the same name in
  existence.)

- Steps for setting up Eclipse under Ubuntu Linux 14.04 can be found in
  [Eclipse-readme](docs/Eclipse-readme.md).

# Building

By default the build is performed in a separate folder `build`.

```
./bootstrap.sh
cd build
make -j4
make check -j4
```

We recommend using `clang++` with no optimizations for speeding up
compilation and simplifying debugging.

## Additional documentation

More documentation is in [docs/README.md](docs/README.md)
