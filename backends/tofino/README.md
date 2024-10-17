# Overview of bf-p4c-compilers

## Documentation

This Doxygen-based documentation should serve as a documentation of the bf-p4c-compilers
code base for compiler developers. For this purpose, documentation for private members
is also enabled (EXTRACT_PRIVATE = YES).

The documentation should cover the gap between the microarchitecture documents
and presentations (e.g. bootcamps). It should explain how the code maps to the
hardware architecture, while it should not duplicate the contents of the
microarchitecture documents.

One of the most important contributions is to write high-level description of compiler topics.
For this purpose, Doxygen "groups" are used. Whatever is included in a particular group,
it appears under the corresponding "Modules" section. Put the \\defgroup command in the most
suitable file for the description of a particular topic. Then, each class that should be
included is marked with the \\ingroup command.

Don't forget to keep .dockerignore and the INPUT tag synchronized in order to include
desired .md files.

### Best practices for using Doxygen

- Use \@brief to introduce a brief, one-sentence description, which is used in listings.
- Use \@p to refer to a method's parameters in running text
  (as opposed to \@param to describe the parameter).
- Use \@a to refer to a struct/class' member in running text.
- To avoid automatic link generation for a particular word, prepend it with \@.
- There is no \@seealso, use \@sa or \@see instead.

### Using Doxygen in Python 3 scripts

Doxygen can also parse Python 3 documentation comments. For this the usual Python 3 
doc strings can be used, but each Doxygen documentation comment has to have `!` at the 
start of the comment.

Example documentation comment:
```
def foo(bar):
    """! Does foo
    @param bar Bar object
    @return None
    """
    ...
```

## Setup

This repo contains the backend for the Barefoot p4c compiler suite.
It has the following structure:

```
bf-p4c-compilers
├── p4c      -- submodule for the p4lang/p4c repo
├── bf-p4c   -- the contents of the old p4c-extension-tofino repo
├── bf-asm   -- the contents of the old tofino-asm repo
└-- p4_tests -- submodule for the p4_tests repo
```

There are two scripts in the top level directory that support building
the compiler and assembler.

bootstrap_bfn_env.sh -- used to checkout and build all dependent
repositories and install dependent packages

- some systems may see issues with conflicts between different versions
  of libproto.so -- when running the above bootstrap_bfn_env.sh script you
  may see warnings about conflicting libproto.so versions followed by
  incomprehensible linking errors.  To fix this you need to remove the
  symlinks/static libs for the old versions of libproto so only the newer
  ones will be used by the linker.  On Ubuntu 16.04 you need:
```
    $ cd /usr/lib/x86_64-linux-gnu
    $ sudo mkdir old
    $ sudo mv libproto*.so old
    $ sudo mv libproto*.a old
```
- you may choose to remove the files rather than just moving them, but DO NOT
  remove the versioned .so files (libprotobuf.so.9.0.1 and others), as doing
  so will break your system.

bootstrap_bfn_compilers.sh -- bootstraps the configuration for p4c
(with the Tofino extension) and assembler.

To configure and build:
```
git clone --recursive git@github.com:barefootnetworks/bf-p4c-compilers.git
cd bf-p4c-compilers
./bootstrap_bfn_env.sh
./bootstrap_bfn_compilers.sh [--prefix <path>] [--enable-doxygen-docs]
cd build
make -j N [check] [install]
```

## Dependencies

Each of the submodules have dependencies. Please see the
[p4lang/p4c](p4c/README.md), [Barefoot p4c](bf-p4c/README.md), and
[Barefoot asm](bf-asm/README.md) for the different project
dependencies.
