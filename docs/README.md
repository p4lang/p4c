# Repository

This folder contains documentation for the P4_16 prototype compiler.
The code and documentation are hosted in the following repository: https://github.com/p4lang/p4c

# Compiler source code organization

```
p4c
├── build                     -- recommended place to build binary
├── p4include                 -- standard P4 files needed by the compiler
├── backends
│   ├── p4test                -- "fake" back-end for testing
│   ├── ebpf                  -- extended Berkeley Packet Filters back-end
│   └── bmv2                  -- behavioral model version 2 (switch simulator) back-end
├── extensions
│   └── XXXX                  -- symlinks to custom back-ends
├── docs                      -- documentation
├── frontends
│   ├── common                -- common front-end utilities
│   ├── p4-14                 -- P4_14 front-end
│   └── p4                    -- P4_16 front-end
├── ir                        -- core internal representation
├── lib                       -- common utilities (libp4toolkit.a)
├── midend                    -- code that may be useful for writing mid-ends
├── test                      -- test code
│   └── unittests             -- unit test code
├── tools                     -- external programs used in the build/test process
│   └── ir-generator          -- code for the IR C++ class hierarchy generator
└── testdata                  -- test inputs and reference outputs
    ├── p4_16_samples         -- P4_16 input test programs
    ├── p4_16_errors          -- P4_16 negative input test programs
    ├── p4_16_samples_outputs -- Expected outputs from P4_16 tests
    ├── p4_16_errors_outputs  -- Expected outputs from P4_16 negative tests
    ├── v1_1_samples          -- P4 v1.1 sample programs
    ├── p4_14_samples         -- P4_14 input test programs
    ├── p4_14_samples_outputs -- Expected outputs from P4_14 tests
    └── p4_14_errors          -- P4_14 negative input test programs
```

# Dependences

We have tested the compiler on U*X systems (OS X and Ubuntu).  The
following tools are required to build and run the compiler and tests:

- A C++11 compiler
  E.g., gcc 4.8 or later, or clang++

- `git` for version control

- GNU autotools for the build process

- Boehm-Weiser garbage-collector C++ library

- Gnu Bison and Gnu Flex

- Gnu multiple precision library GMP

- C++ boost library (minimally used)

- Python 2.7 for scripting (especially for running tests)

Note that each back-end may have additional dependences:
  * [BMv2](../backends/bmv2/README.md)
  * [eBPF](../backends/ebpf/README.md)

## Ubuntu dependences

Most dependences can be installed using `apt-get install`:

`sudo apt-get install libgc-dev bison flex libgmp-dev libboost-all-dev`

## OS X dependences

Installing dependences on OS X:

- You may need to install brew for useful utilities:
  `sudo /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"`

- `sudo brew install autoconf automake libtool boost`

- You can recompile the C++ garbage-collector from sources from `https://github.com/ivmai/bdwgc.git`

- You can recompile GMP from sources `https://gmplib.org/download/gmp/gmp-6.1.0.tar.bz2` (you will need
  to `configure --enable-cxx`)

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
  [Eclipse-readme](Eclipse-readme.md).

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

# Additional documentation

* the P4_14 (P4 v1.0) language is described in the P4 spec:
  http://p4.org/wp-content/uploads/2015/04/p4-latest.pdf

* the P4_16 draft language is described in [this Word
  document](https://github.com/p4lang/p4-spec/tree/master/p4_16/spec/P4-16-draft-spec.docx).
  This language is still under design.

* the compiler intermediate representation (IR) is briefly described
  in [IR](IR.md)

* The [migration guide](migration-guide.pptx) describes how P4_14 (v1.0)
  programs are translated into P4_16 programs

* The [compiler design](compiler-design.pptx) describes the salient
  features of the compiler design and implementation

* The [open issues](TODO.md) document describes work that is still to
  be performed

* specific back-ends may have their own README files; check the
  `extensions` sub-folders.

  * [BMv2](../backends/bmv2/README.md)
  * [eBPF](../backends/ebpf/README.md)

# Coding conventions

* Coding style is guided by the [following
  rules](CodingStandardPhilosophy.md)

* The compiler uses several of the [Google C++ coding style
  guidelines](https://google.github.io/styleguide/cppguide.html), but
  not all.  We have customized Google's `cpplint.py` tool for our
  purposes.  The tool can be invoked with `make cpplint`.

* watch out for `const`; it is very important.

* use `override` whenever possible; it may catch errors early

* never use `const_cast` and `reinterpret_cast`.

* The C++ code is written to use a garbage-collector
  * do not use any smart pointers, just raw pointers
* Use our implementations and wrappers instead of standard classes:

  * use `cstring` for constant strings.  For java programmers, `cstring`
    should be used where you would use java.lang.String, and `std::string`
    should be used where you would use StringBuilder or StringBuffer.

  * use the `BUG()` macro to signal an exception.  This macro is
    guaranteed to throw an exception.

  * use `CHECK_NULL()` to validate that pointers are not nullptr

  * use BUG_CHECK() instead of `assert`, and always supply an
    informative error message

  * use `::error()` and `::warning()` for error reporting.  They use the
    `boost::format` for the format argument, which has some compatibility
    for `printf` arguments.   These functions handle IR and SourceInfo
    objects smartly.  Here is an example:

```C++
IR::NamedRef *ref;
error("%1%: No header or metadata named '%2%'", ref->srcInfo, ref->name);
```

output:

```
../testdata/v1_errors/missing_decls1.p4(6): Error: No header or metadata named 'data'
    if (data.b2 == 0) {
        ^^^^
```

  * use `vector` and `array` wrappers for `std::vector` and `std::array`
    (these do bounds checking on all accesses).

# How to contribute

* do write unit test code
* code has to be reviewed before it is merged
* make sure all tests pass when you send a pull request (only PASS tests allowed)
* make sure `make cpplint` produces no errors

# Git usage

* Fork the p4lang/p4c repository on github
  (see https://help.github.com/articles/fork-a-repo/)
* After committing changes, create a pull request and assign it to ChrisDodd
* Incorporate all review comments.

# Debugging

* To debug the build process you can run `make V=1`

* The top-level `.gdbinit` file has some additional pretty-printers.
  If you start gdb in this folder (p4c), then it should be
  automatically used.  Otherwise you can run at the gdb prompt `source
  path-to-p4c/.gdbinit`.

* To debug the compiler parser you can set the environment variable
  `YYDEBUG` to 1

* The following methods can be used to print nice representations of
  compiler data structures:

  * `void dbprint(std::ostream& out) const`: this method is used when
    logging information.  It should print useful debug information,
    intended for consumption by compiler writers.

  * `cstring toString() const`: this method is used when reporting
    error messages to compiler users.  It should only display
    information that is related to the P4 user program, and never
    internal compiler data structures.

* Use the LOG* macros for writing debug messages.  gdb misbehaves
  frequently, so log messages are the best way to debug your programs.
  The number in the function name is the debug verbosity.  The higher,
  the less important the message.  This macro invokes the `dbprint`
  method on objects that provide it.  Here is an example usage:
  `LOG1("Replacing " << id << " with " << newid);`

* You can control the logging level per compiler source-file with the
  `-T` compiler command-line flag.  The flag is followed by a list of
  file patterns and a numeric level after a colon `:`.  This flag
  enables all logging messages above the specified level for all
  compiler source files that match the file pattern.

  For example, to enable logging in file `node.cpp` above level 1, and
  in file `pass_manager.cpp` above level 2, use the following compiler
  command-line option: `-Tnode:1,pass_manager:2`

# Testing

The testing infrastructure is based on autotools.  We use several
small python and shell scripts to work around limitations of
autotools.

* To run tests execute `make check`
  - There should be no FAIL or XPASS tests.
  - XFAIL tests are tolerated only transiently - these indicate known
  unfixed bugs in the compiler.

* To run a subset of tests execute `make check-PATTERN`.  E.g., `make
  check-p4`.

* Add unit tests in `test/unittests`

* Code for running various compiler back-ends on p4 files is generated
  using a simple python script `tools/gen-tests.py`.
