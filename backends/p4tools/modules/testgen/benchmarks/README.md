<!--
SPDX-FileCopyrightText: 2023 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

<!-- 
Documentation Inclusion:
This README is integrated as a subsection of the "P4Testgen" page in the P4 compiler documentation.

Refer to the specific section here: [P4Testgen Benchmarks - Subsection](https://p4lang.github.io/p4c/p4testgen.html#p4testgen-bmv2-target-tests)
-->
# P4Testgen Benchmarks
The [`backends\p4tools\modules\testgen\benchmarks` folder](https://github.com/p4lang/p4c/tree/main/backends/p4tools/modules/testgen/benchmarks) contains utility scripts to benchmark P4Testgen. `test_coverage.py` measures coverage of various path selection strategies. `plot.py` creates plots of the results.

`memory.py` measures peak resident memory at increasing test counts. It requires
Linux and GNU `/usr/bin/time`. Use the same program, seed, and build configuration
when comparing binaries, changing only `ENABLE_GC` (or `DISABLE_GC`). For example:

```sh
python3 backends/p4tools/modules/testgen/benchmarks/memory.py \
  --binary build-gc/p4testgen --binary build-no-gc/p4testgen \
  --program testdata/p4_16_samples/pins/pins_middleblock.p4 \
  --counts 100 1000 10000 --out-dir /tmp/testgen-memory
```

The output directory must be new. Each run records its command, exit status,
generated test count, elapsed time, and peak RSS in KiB in `results.json`, with
compiler logs and generated STF tests in separate directories. An error, timeout,
or failure to generate the requested number of tests makes the benchmark fail.
Use `--max-peak-rss-mib` to make exceeding a memory budget fail the benchmark.
The no-GC CI job uses a generous 1 GiB ceiling to catch large regressions; local
comparisons should also inspect how RSS changes with test count.
RSS measurements complement sanitizer leak checks; bounded RSS alone does not
establish that every allocation is released.
The no-GC CI job also runs Valgrind on a compiler IR JSON round trip and a
10-test middleblock run, failing on invalid accesses and definite or indirect
leaks. Its memory artifact includes these logs.

Check a smaller run with Valgrind to locate leaked allocations and invalid memory
accesses (use a no-GC build with debug information):

```sh
valgrind --leak-check=full --show-leak-kinds=definite,indirect \
  --errors-for-leak-kinds=definite,indirect --error-exitcode=1 \
  build-no-gc/p4testgen --target bmv2 --arch v1model --test-backend STF \
  --seed 1000 --max-tests 100 --out-dir /tmp/testgen-valgrind \
  testdata/p4_16_samples/pins/pins_middleblock.p4
```
