# P4Testgen - Generating Tests for P4 Targets

## Directory structure

```
testgen
 ├─ cmake       ── common CMake modules
 ├─ common      ── C++ source: common code for the various components of p4check
 ├─ mutate      ── C++ source: p4mutate
 ├─ p4check     ── C++ source: the main entry point for p4check
 ├─ scripts
 │   ├─ hooks   ── git hooks
 │   └─ tools   ── development-support scripts (e.g., cpplint)
 ├─ smith       ── C++ source: p4smith
 ├─ submodules  ── external dependencies
 └─ testgen     ── C++ source: p4testgen
```

## Building

P4Testgen can be built using the following CMAKE configuration in the P4C repository.

```
mkdir build
cd build
cmake .. -DENABLE_GMP=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_P4TOOLS=ON
make
```

### Running ctest
Once compiled, run ctest in the build directory:
```
ctest -V -R <test_name>
```
Testgen tests will have the prefix `testgen-`.

## Usage
The main binary can be found in `build/p4check`.

To generate tests for a particular target and P4 architecture, run `p4check testgen –target [TARGET] –arch [ARCH] –max-tests 10 –out-dir [OUT] prog.p4`
`p4check` is an umbrella binary which delegates execution to all tools of the p4-tools repository.
`testgen` specifies that the testgen tool should be used. In the future, other tools may be supported, for example random program generation or translation validators.
These are the current usage flags:

```bash
./p4check: Generate packet tests for a P4 program
--help                     Shows this help message and exits
--version                  Prints version information and exits
--min-packet-size bytes    Sets the minimum allowed packet size, in bytes. Any packet shorter than this is considered to be invalid, and will be dropped if the program would otherwise send the packet on the network.
--mtu bytes                Sets the network's MTU, in bytes
--seed seed                Provides a randomization seed
-I path                    Adds the given path to the preprocessor include path
-D arg=value               Defines a preprocessor symbol
-U arg                     Undefines a preprocessor symbol
-E                         Preprocess only. Prints preprocessed program on stdout.
--nocpp                    Skips the preprocessor; assumes the input file is already preprocessed.
--std {p4-14|p4-16}        Specifies source language version.
-T loglevel                Adjusts logging level per file.
--target target            Specifies the device targeted by the program.
--arch arch                Specifies the architecture targeted by the program.
--top4 pass1[,pass2]       Dump the P4 representation after
                           passes whose name contains one of `passX' substrings.
                           When '-v' is used this will include the compiler IR.
--dump folder              Folder where P4 programs are dumped.
-v                         Increase verbosity level (can be repeated)
--input-packet-only        Only produce the input packet for each test
--max-tests maxTests       Sets the maximum number of tests to be generated
--out-dir outputDir        Directory for generated tests
--test-backend             Select a test back end. Available test back ends are defined by the respective target.
--packet-size packetSize   If enabled, sets all input packets to a fixed size in bits (from 1 to 12000 bits). 0 implies no packet sizing.
--pop-level                This is the fraction of unexploredBranches we select on multiPop. Defaults to 0.
--linear-enumeration       Max bound for LinearEnumeration strategy. Defaults to 0. **Experimental feature**.
```

Once P4Testgen has generated tests, the tests can be executed by either the P4Runtime or STF test back ends.

## Dependencies
* [inja](https://github.com/pantor/inja) template engine for testcase generation.
* [gsl-lite](https://github.com/gsl-lite/gsl-lite) C++ Core Guidelines Support Library, used to
support and enforce C++ best practices.

## Development Style
Currently, each C++ source directory has a few subdirectories, including:
* `core`, containing the core functionality of the submodule; and
* `lib`, containing supporting data structures.

The distinction between the two can be fuzzy. Here are some guiding principles
for where to find/put what:
* If it depends on anything in `core`, it belongs in `core`.
* If it's something that resembles a general-purpose data structure (e.g., an
  environment or a symbol table), it's probably in `lib`.

### C++ Coding style

We generally follow the [Google C++ Style
Guide](https://google.github.io/styleguide/cppguide.html). This is partially
enforced by `cpplint` and `clang-format` and their respective configuration files. Both tools run in a git hook and as part of CI. To run `clang-format` on Ubuntu 16.04, install it with `pip3 install --user clang-format`.

Some deviations from the Style Guide are highlighted below. While this
repository depends heavily on the main P4 compiler ([P4C](https://github.com/p4lang/p4c/)), the code there doesn't always follow our style; don't follow its precedent!

* Comments are important. The Style Guide's [section on
  comments](https://google.github.io/styleguide/cppguide.html#Comments) is
  required reading.
    * Classes, methods, and fields are documented with triple-slash
      Doxygen-style comments:
      ```
      /// An example class demonstrating a documentation comment.
      class C {};
      ```
    * We do not use copyright headers or license boilerplate in our source
      files. Where needed, these will be auto-generated during release
      packaging.
* Lines are wrapped at 100 characters.
* Indents are four spaces. Tab characters should not be used for indenting.
* Generally prefer a single class declaration per `.h` file, unless providing a
  library of related classes. Multiple classes may be declared in a `.cpp`
  file.
* Include headers in the following order: related header, C system headers, C++
  system headers, other library headers, `P4Testgen` headers. Each class of
  headers should be sorted alphabetically and separated by a blank line. Files
  from `P4Testgen` should be listed relative to the project root. Files from
  `p4c` should be listed relative to the submodule's root.

  For example, in `testgen/lib/foo.cpp`, format your includes as follows.
  (Note: the comments below are for explanatory purposes, and should be elided
  in the actual file.)
  ```
  // Related header
  #include "backends/p4tools/lib/foo.h"

  // C system headers
  #include <stdlib.h>
  #include <time.h>

  // C++ system headers
  #include <boost/optional.hpp>
  #include <set>
  #include <vector>

  // Other library headers
  #include "lib/error_type.h"
  #include "gsl/gsl-lite.hpp"
  #include "ir.h"

  // Headers from P4Testgen folders
  #include "backends/p4tools/testgen/common/lib/model.h"
  #include "backends/p4tools/testgen/lib/symbolic_env.h"
  ```



