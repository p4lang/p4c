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
│   ├── common                -- common front-end code
│   ├── p4-14                 -- P4_14 front-end
│   └── p4                    -- P4_16 front-end
├── ir                        -- core internal representation
├── lib                       -- common utilities (libp4toolkit.a)
├── midend                    -- code that may be useful for writing mid-ends
├── p4include                 -- p4 include files (e.g., core.p4)
├── test                      -- test code
│   └── unittests             -- unit test code
├── tools                     -- external programs used in the build/test process
│   └── ir-generator          -- code for the IR C++ class hierarchy generator
└── testdata                  -- test inputs and reference outputs
    ├── p4_16_samples         -- P4_16 input test programs
    ├── p4_16_errors          -- P4_16 negative input test programs
    ├── p4_16_samples_outputs -- Expected outputs from P4_16 tests
    ├── p4_16_errors_outputs  -- Expected outputs from P4_16 negative tests
    ├── p4_16_bmv_errors      -- P4_16 negative input tests for the bmv2 backend
    ├── v1_1_samples          -- P4 v1.1 sample programs
    ├── p4_14_samples         -- P4_14 input test programs
    ├── p4_14_samples_outputs -- Expected outputs from P4_14 tests
    └── p4_14_errors          -- P4_14 negative input test programs
```

# Additional documentation

* the P4_14 (P4 v1.0) language is described in the P4 spec:
  http://p4.org/wp-content/uploads/2015/04/p4-latest.pdf

* the P4_16 draft language is described in [this pdf
  document](http://p4.org/wp-content/uploads/2016/12/P4_16-prerelease-Dec_16.pdf).
  This language is still under revision.

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

# How to contribute

* do write unit test code
* code has to be reviewed before it is merged
* make sure all tests pass when you send a pull request (only PASS tests allowed)
* make sure `make cpplint` produces no errors

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

## Testing

The testing infrastructure is based on autotools.  We use several
small python and shell scripts to work around limitations of
autotools.

* To run tests execute `make check -j3`
  - There should be no FAIL or XPASS tests.
  - XFAIL tests are tolerated only transiently - these indicate known
  unfixed bugs in the compiler.

* To run a subset of tests execute `make check T='-k PATTERN'`.  E.g., `make
  check T='-k bmv2'` or `make check T='-k foo.p4'`.

* To rerun the tests that failed last time run `make recheck -j3`

* Add unit tests in `tests`

* Code for running various compiler back-ends on p4 files is generated
  using a simple python script `tools/gen-tests.py`.

* See make check-help for more options

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
