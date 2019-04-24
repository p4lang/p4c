# Repository

This folder contains documentation for the P4_16 prototype compiler.
The code and documentation are hosted in the following repository: https://github.com/p4lang/p4c

# Compiler source code organization

```
p4c
├── build                     -- recommended place to build binary
├── backends
│   ├── p4test                -- "fake" back-end for testing
│   ├── ebpf                  -- extended Berkeley Packet Filters back-end
│   ├── graphs                -- backend that can draw graphiz graphs of P4 programs
│   └── bmv2                  -- behavioral model version 2 (switch simulator) back-end
├── control-plane             -- control plane API
├── docs                      -- documentation
│   └── doxygen               -- documentation generation support
├── extensions
│   └── XXXX                  -- symlinks to custom back-ends
├── frontends
│   ├── common                -- common front-end code
│   ├── parsers               -- parser and lexer code for P4_14 and P4_16
│   ├── p4-14                 -- P4_14 front-end
│   └── p4                    -- P4_16 front-end
├── ir                        -- core internal representation
├── lib                       -- common utilities (libp4toolkit.a)
├── midend                    -- code that may be useful for writing mid-ends
├── p4include                 -- standard P4 files needed by the compiler (e.g., core.p4)
├── test                      -- test code
│   └── gtest                 -- unit test code written using gtest
├── tools                     -- external programs used in the build/test process
│   ├── driver                -- p4c compiler driver: a script that invokes various compilers
│   ├── stf                   -- Python code to parse STF files (used for testing P4 programs)
|   └── ir-generator          -- code for the IR C++ class hierarchy generator
└── testdata                  -- test inputs and reference outputs
    ├── p4_16_samples         -- P4_16 input test programs
    ├── p4_16_errors          -- P4_16 negative input test programs
    ├── p4_16_samples_outputs -- Expected outputs from P4_16 tests
    ├── p4_16_errors_outputs  -- Expected outputs from P4_16 negative tests
    ├── p4_16_bmv_errors      -- P4_16 negative input tests for the bmv2 backend
    ├── v1_1_samples          -- P4 v1.1 sample programs
    ├── p4_14_errors          -- P4_14 negative input test programs
    ├── p4_14_errors_outputs  -- Expected outputs from P4_14 negative tests
    ├── p4_14_samples         -- P4_14 input test programs
    ├── p4_14_samples_outputs -- Expected outputs from P4_14 tests
    └── p4_14_errors          -- P4_14 negative input test programs
```

# Additional documentation

* the P4_14 and P4_16 languages are described in their respective
  specifications, available [here](https://p4.org/specs).

* the core design of the compiler intermediate representation (IR) and
  the visitor patterns are briefly described in [IR](IR.md)

* The [migration guide](migration-guide.pptx) describes how P4_14 (v1.0)
  programs are translated into P4_16 programs

* The [compiler design](compiler-design.pptx) describes the salient
  features of the compiler design and implementation; this document has several subsections:
  * Compiler goals
  * Compiler architecture
  * Source code organization
  * IR and visitors; recipes
  * A guide to the existing passes
  * Discussion of the three sample back-ends

* Specific back-ends may have their own documentation; check the
  `extensions` sub-folders, and also the following supplied back-ends:
  * [BMv2](../backends/bmv2/README.md)
  * [eBPF](../backends/ebpf/README.md)

* Check out the [IntelliJ P4 plugin](https://github.com/TakeshiTseng/IntelliJ-P4-Plugin)

# How to contribute

* do write unit test code
* code has to be reviewed before it is merged
* make sure all tests pass when you send a pull request
* make sure `make cpplint` produces no errors (`make check` will also run this)
* write documentation

# Writing documentation

Documenting the workings of the compiler is a never-ending (many times
overlooked) job. We can always write better documentation!

In P4C, documentation is generated using Doxygen
(http://www.stack.nl/~dimitri/doxygen/index.html). There are two main
sources from which we generate documentation: comments in the code and
markup documents in the docs/doxygen directory.

Code comments should capture the main intent of the implementation and
the "why", rather than the "how". The how can be read from the code,
however, documenting the reasons why a certain implementation was
chosen will help other contributors understand the design choices and
enable them to reuse your code. Also important in the context of the
compiler is to document the invariants for each pass (or groups of
passes), since it is likely that other developers will need to insert
additional passes, and they should understand the effects that the
pass ordering has on the AST.

Documentation in the markup documents is intended for higher level
design documentation. The files will be automatically captured in the
documentation in the order implied by their naming: XX_my_doc.md where
XX is a number between 02-99. Currently, 00_revision_history.md
contains the documentation revision history, and 01_overview.md is the
overview of the compiler goals and architecture.

Happy writing! Should you have any questions, please don't hesitate to ask.

## Git usage

* To contribute: fork the p4lang/p4c repository on github
  (see https://help.github.com/articles/fork-a-repo/)
* To merge a forked repository with the latest changes in the source use:

```
git fetch upstream
git rebase upstream/master
git push -f
```

* After committing changes, create a pull request (using the github web UI)

* Follow these
  [guidelines](CodingStandardPhilosophy.md#Git-commits-and-pull-requests)
  to write commit messages and open pull requests.

## Debugging

* To debug the build process you can run `make V=1`

* The top-level `.gdbinit` file has some additional pretty-printers.
  If you start gdb in this folder (p4c), then it should be
  automatically used.  Otherwise you can run at the gdb prompt `source
  path-to-p4c/.gdbinit`.

* To debug the compiler parser you can set the environment variable
  `YYDEBUG` to 1

* The following `IR::Node` methods can be used to print nice representations of
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

* Keep the compiler output deterministic; watch for iterators over
  sets and maps, which may introduce non-deterministic orders.  Use
  our own `ordered_map` and `ordered_set` if you iterate, to keep
  iteration order deterministic.

* You can control the logging level per compiler source-file with the
  `-T` compiler command-line flag.  The flag is followed by a list of
  file patterns and a numeric level after a colon `:`.  This flag
  enables all logging messages above the specified level for all
  compiler source files that match the file pattern.

  For example, to enable logging in file `node.cpp` above level 1, and
  in file `pass_manager.cpp` above level 2, use the following compiler
  command-line option: `-Tnode:1,pass_manager:2`

  To execute LOG statements in a header file you must supply the complete
  name of the header file, e.g.: `-TfunctionsInlining.h:3`.

## Testing

The testing infrastructure is based on small python and shell scripts.

* To run tests execute `make check -j3`
  - There should be no FAIL or XPASS tests.
  - XFAIL tests are tolerated only transiently.

* To run a subset of tests execute `make check-PATTERN`.  E.g., `make
  check-p4`.

* To rerun the tests that failed last time run `make recheck`

* Add unit tests in `test/gtest`

Test programs with file names ending in `-bmv2.p4` or `-ebpf.p4` may
have an STF (Simple Test Framework) file with file name suffix `.stf`
associated with them.  If the machine on which you are running has a
copy of `simple_switch` or the EBPF software switch installed, not
only will those programs be compiled for those targets, but also table
entries optionally specified in the STF file will be installed, and
input packets will be sent to the data plane and output packets
checked against expected packets in the STF file.

When pull requests are created on the p4c Github repository, the
changes are built, and the tests executed via `make check`.  These
tests are run with a "recently built" version of `simple_switch` from
the
[p4lang/behavioral-model](https://github.com/p4lang/behavioral-model)
repository, but it can be several hours old.  If you are working on
p4c features that rely on newly committed changes to `simple_switch`
you can find out which `simple_switch` version these p4c automated
tests are using at the link below:

+ [https://hub.docker.com/r/p4lang/behavioral-model/builds](https://hub.docker.com/r/p4lang/behavioral-model/builds)

## Coding conventions

* Coding style is guided by the [following
  rules](CodingStandardPhilosophy.md)

* We use several (but not all) of the [Google C++ coding style
  guidelines](https://google.github.io/styleguide/cppguide.html).
  We have customized Google's `cpplint.py` tool for our
  purposes.  The tool can be invoked with `make cpplint`.

* watch out for `const`; it is very important.

* use `override` whenever possible (new gcc versions enforce this)

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

  * use `BUG_CHECK()` instead of `assert`, and always supply an
    informative error message

  * use `::error()` and `::warning()` for error reporting. See the
    [guidelines](CodingStandardPhilosophy.md#Handling-errors) for more
    details.

  * use `LOGn()` for log messages -- the `n` is an integer constant for
    verbosity level.  These can be controlled on a per-source-file basis
    with the -T option.  LOG1 should be used for general messages, so that
    running with -T*:1 (turning on all LOG1 messages) is not too overwhelming.
    LOG2 should be used to print information about the results of a module
    that later passes may need to debug them.  Details of what a module
    or pass is doing and looking at (only of interest when debugging that
    code) should be at LOG4 or higher.

  * use the `vector` and `array` wrappers for `std::vector` and `std::array`
    (these do bounds checking on all accesses).

  * use `ordered_map` and `ordered_set` when you need to iterate;
    they provide deterministic iterators

## Compiler Driver

**p4c** is a compiler driver. The goal is to provide a consistent user interface
across different p4 backends and work flows. The compiler driver is written in
Python. It can be extended for custom backends.

The usage of the driver is as follows:
```
usage: p4c [-h] [-V] [-v] [-###] [-Xpreprocessor <arg>] [-Xp4c <arg>]
           [-Xassembler <arg>] [-Xlinker <arg>] [-b BACKEND] [-E] [-e] [-S]
           [-c] [-x {p4-14,p4-16}] [-I SEARCH_PATH] [-o PATH] [--target-help]
           [source_file]

positional arguments:
  source_file           File to compile

optional arguments:
  -h, --help            show this help message and exit
  -V, --version         show version and exit
  -v                    verbose
  -###                  print (but do not run) the commands
  -Xpreprocessor <arg>  Pass <arg> to the preprocessor
  -Xp4c <arg>           Pass <arg> to the compiler
  -Xassembler <arg>     Pass <arg> to the assembler
  -Xlinker <arg>        Pass <arg> to the linker
  -b BACKEND            specify target backend
  -E                    Only run the preprocessor
  -e                    Skip the preprocessor
  -S                    Only run the preprocess and compilation steps
  -c                    Only run preprocess, compile, and assemble steps
  -x {p4-14,p4-16}      Treat subsequent input file as having type language.
  -I SEARCH_PATH        Add directory to include search path
  -o PATH               Write output to the provided path
  --target-help         Display target specific command line options.
```

To extend the driver, user needs to create a configuration file and add it to the `p4c_PYTHON`
makefile variable.

```
# In your custom Makefile.am

p4c_PYTHON += p4c.custom.cfg
```

There is an global variable `config` in p4c compiler driver that stores the build steps
for a particular target. By default, the bmv2 and ebpf backends are supported. Each backend
is identified with a triplet: **target-arch-vendor**. For example, the default bmv2 backend is
identified as `bmv2-ss-p4org`. Users may choose to implement different architectures running
on the same target, and they should configure the compilation flow as follows:

```
config.add_preprocessor_options("bmv2-newarch-p4org", "-E")
config.add_compiler_options("bmv2-newarch-p4org", "{}/{}.o".format(output_dir, source_basename))
config.add_assembler_options("bmv2-newarch-p4org", "{}/{}.asm".format(output_dir, source_basename))
config.add_linker_options("bmv2-newarch-p4org", "{}/{}.json".format(output_dir, source_basename))

config.add_toolchain("bmv2-newarch-p4org", {"preprocessor": "cc", "compiler": "p4c-bm2-newarch", "assembler": "", "linker": ""})
config.add_compilation_steps(["preprocessor", "compiler"])
config.target.append("bmv2-newarch-p4org")
```

After adding the new configuration file, rerun `bootstrap.sh`

For testing purpose, p4c will be installed in the build/ directory when executing `make`.
User can install `p4c` to other system path by running `make install`
