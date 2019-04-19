# Common Utilities

This directory contains miscellaneous utilities that are generally useful
and not specific to any part of the compiler.  Most are not even compiler
specific.  The files in this folder should only depend on each other; they
cannot depend on any other compiler files.

##### algorithm.h

wrapper around `<algorithm>` that contains severla useful additional algorithms

##### bitops.h

bit manipulation operations

##### bitvec.h, bitvec.cpp

dynamic bitvectors with useful operations.  The standard types `std::vector<bool>` and
`std::bitsec` are missing crucial functionality, making them generally useless.

##### cstring.h, cstring.cpp

constant strings.  The standard library `std::string` type is mutable, allowing the
string to be changed dynamically.  Constant strings are what we would prefer to use
in the compiler.  `cstring` keeps the memory for all constant strings in a single
global pool, allowing constant time comparisons.

##### default.h

Synthesizing default values of various types (e.g., 0 for integers,
nullptr for pointers, etc.)

##### enumerator.h, enumerator.cpp

C#-like enumerator interface.

##### error.h, error.cpp, expresions.h

Error reporting functions.

##### gc.cpp

Overrides global `operator new` and `delete` to use the Boehm/Demers/Weiser conservative
collector, so all memory allocations are garbage collected

##### hex.h, hex.cpp

Adaptor for more conveniently printing hexadecimal string with ostreams.

##### indent.h, indent.cpp

Adaptor for managing indentation on ostreams

##### log.h, log.cpp

Macros and support for logging that can be managed on a per-source-file basis

##### ltbitmatrix.h

Adaptor using a `bitvec` as a lower-triangular bit matrix.

##### map.h

Wrapper around `map`, adding some useful functions that are missing from `std::map`

##### nullstream.h

A simple ostream that does nothing.

##### options.h, options.cpp

Represents compiler command-line options.

##### path.h, path.cpp

Simple system-independent pathname abstraction.

##### range.h

Iterators over numeric ranges.

##### source_file.h, source_file.cpp

Represents the input source of the compiler and source file position
information used for error reporting and generating debugging information.

##### stringify.h, stringify.cppp

Conversion of various types to strings.

##### sourceCodeBuilder.h

Support for emitting programs in source (works for P4 and C).

##### stringref.h

A stringref is really a substring of another string, with a specified
length, but sharing the same storage.  Since it is just a reference, care needs
to be taken to ensure that it does not outlive the storage it refers to -- if
the object owning the memory releases it.
