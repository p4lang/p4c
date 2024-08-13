# Common P4C utility functions

The [lib directory](https://github.com/p4lang/p4c/tree/main/lib) contains miscellaneous utilities that are generally useful
and not specific to any part of the compiler.  Most are not even compiler
specific.  The files in lib folder should only depend on each other; they
cannot depend on any other compiler files.

| File(s)                      | Description |
|------------------------------|-------------|
| `algorithm.h`                | Wrapper around `<algorithm>` that contains several useful additional algorithms. |
| `bitops.h`                   | Bit manipulation operations. |
| `bitvec.h`, `bitvec.cpp`     | Dynamic bitvectors with useful operations. The standard types `std::vector<bool>` and `std::bitset` are missing crucial functionality, making them generally useless. |
| `cstring.h`, `cstring.cpp`   | Constant strings. The standard library `std::string` type is mutable, allowing the string to be changed dynamically. `cstring` keeps the memory for all constant strings in a single global pool, allowing constant time comparisons. |
| `default.h`                  | Synthesizing default values of various types (e.g., 0 for integers, `nullptr` for pointers, etc.). |
| `enumerator.h`, `enumerator.cpp` | C#-like enumerator interface. |
| `error.h`, `error.cpp`, `expressions.h` | Error reporting functions. |
| `gc.cpp`                     | Overrides global `operator new` and `delete` to use the Boehm/Demers/Weiser conservative collector, so all memory allocations are garbage collected. |
| `hex.h`, `hex.cpp`           | Adaptor for more conveniently printing hexadecimal strings with ostreams. |
| `indent.h`, `indent.cpp`     | Adaptor for managing indentation on ostreams. |
| `log.h`, `log.cpp`           | Macros and support for logging that can be managed on a per-source-file basis. |
| `ltbitmatrix.h`              | Adaptor using a `bitvec` as a lower-triangular bit matrix. |
| `map.h`                      | Wrapper around `map`, adding some useful functions that are missing from `std::map`. |
| `nullstream.h`               | A simple ostream that does nothing. |
| `options.h`, `options.cpp`   | Represents compiler command-line options. |
| `range.h`                    | Iterators over numeric ranges. |
| `source_file.h`, `source_file.cpp` | Represents the input source of the compiler and source file position information used for error reporting and generating debugging information. |
| `stringify.h`, `stringify.cpp` | Conversion of various types to strings.  |
| `sourceCodeBuilder.h`      | Support for emitting programs in source (works for P4 and C).  |
