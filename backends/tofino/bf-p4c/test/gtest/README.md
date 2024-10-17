Using GTEST
===========

We use Google's [gtest](https://github.com/google/googletest) framework for
unit tests.

> NB: There are known issues with include paths for P4 include files.  Until
> these are fixed, you must take care to invoke `gtestp4c` from
> `bf-p4c-compilers/build/p4c`.


Building
--------

The gtest binary is built as part of the default `make` target:

```bash
cd bf-p4c-compilers/build
make
```

For general build instructions, see [here](https://github.com/barefootnetworks/bf-p4c-compilers).

Running
-------

Running `make` produces a `gtestp4c` binary.  To run it, your current working
directory **must** be `build/p4c`.  (This is a known bug.)

```bash
cd bf-p4c-compilers/build/p4c
./test/gtestp4c
```

You can also use `ctest`:

```bash
cd bf-p4c-compilers/build/p4c
ctest -R gtest
```

Finally, the gtest framework provides command line options for selectively
invoking subsets of tests.  Use

```
gtestp4c --help
```

for available options.
