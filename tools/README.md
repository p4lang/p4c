<!--
SPDX-FileCopyrightText: 2022 The P4 Language Consortium

SPDX-License-Identifier: Apache-2.0
-->

# Tools
Contains utility scripts for supporting the development of P4C.

## IWYU

[IWYU](https://github.com/include-what-you-use/include-what-you-use) checks that every symbol used in a C/C++ file has a corresponding include. Vice-versa IWYU also ensures that unnecessary includes are removed.

Since it depends on clang and LLVM, it is recommended to install IWYU from scratch.[Instructions to Build IWYU Standalone](https://github.com/include-what-you-use/include-what-you-use#how-to-build-standalone)

To use IWYU for P4Tools, enable it first in the build order with the option
```
cmake .. -DENABLE_IWYU=ON
```

Then build with the following command, which will collect IWYU's suggestions
```
make 2>&1 | tee iwyu_suggestions.txt
```

The suggestions can then be applied using the fix_includes script
```
fix_includes.py --nocomments --nosafe_headers < iwyu_suggestions.txt
```

For the cleanest results, this sequence of commands should be run multiple times.

### Mappings
Some includes removed or added by IWYU are undesirable. `.imp` include manual mappings that consolidate includes or prevent some includes from being added. Most of these helper files are from the main IWYU repository. To refresh these files run `make refresh` in the `iwyu_mappings folder`.

## clang-format
clang-format checks whether the C/C++ code in this repository is formatted correctly. The appropriate style file is labelled `.clang-format`.

To check whether the code confirms to the correct formatting standard you can run
```
make clang-format
```
in the build directory.
To automatically apply formatting to the entire configured code base run
```
make clang-format-fix-errors
```
in the build directory.

## clang-tidy

C++ semantic analysis uses the pinned clang-tidy 22.1.0 and `.clang-tidy`.
LLVM 22 avoids analyzing system-header code by default. The runner analyzes
translation units in parallel, using each file's actual compilation command.

Prepare a **non-unity** build with CMake 3.18 or newer (enable the backends you
intend to analyze):

```sh
uv run cmake -S . -B build-tidy -G Ninja \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_UNITY_BUILD=OFF -DENABLE_UNIFIED_COMPILATION=OFF
uv run cmake --build build-tidy --target clang-tidy-prepare -j4
uv run python tools/run-clang-tidy.py --build-dir build-tidy --base origin/main -j4
```

The preparation target builds generators and their inputs, not the compiler or
backend executables. Run it again after changing IR definitions, parser grammars,
or protobuf inputs. Analysis fails if a changed source lacks a compilation
command: enable its backend instead of silently dropping coverage.
For Tofino, install `rapidjson-dev` (including its pkg-config metadata) and prepare
with the schema generators' Python dependencies:

```sh
uv run --with pyyaml==6.0.3 --with jsl==0.2.4 --with jsonschema==4.23.0 \
  cmake --build build-tidy --target clang-tidy-prepare -j4
```

Clang 22 rejects an invalid const-member assignment in Ubuntu 24.04's RapidJSON
headers. CI uses fixed headers pinned to RapidJSON commit
`24b5e7a8b27f42fa16b96fc70aade9106cf7102f`, adding their include directory via
`-DCMAKE_CXX_FLAGS="-isystem /absolute/path/to/rapidjson/include"` at configuration.

### CI and existing findings

CI analyzes whole affected translation units and enforces diagnostics on **new
or modified lines**, including changed headers. Existing diagnostics outside
those lines are filtered so adoption does not require a repository-wide cleanup.
This is a location-based filter, not a semantic baseline: a change can cause a
diagnostic at an unchanged line, which requires a full audit to detect.

Source-only changes select those translation units. Header, IR-definition,
protobuf and parser changes conservatively select every translation unit in each configured
profile. Generated parser/IR translation units are included to cover diagnostics
originating in grammars and `.def` files. Tofino's profile includes its Testgen
target as well as the backend. The portable and Tofino profiles each
prepare inputs once, then share generated files and dependency headers with
eight disjoint shards. Each shard uses four workers and a 60-minute job budget.
Every translation unit has a 3600-second limit to accommodate the generated IR
implementation; timeouts and compiler errors fail rather than pass as clean
analysis. Preparation also has a 30-minute limit; the end-to-end time includes
both stages. Per-file timings and diagnostics are uploaded. Per-check
profiling is opt-in with `--profile` to avoid its overhead in routine CI.

No C++ changes means no analysis is necessary. A `.clang-tidy`-only change does
not retroactively enforce all existing findings; validate such changes with a
full audit before proposing them.

Manually dispatching the `p4c-lint` workflow audits every configured translation
unit with the same check policy and shard layout. Manual runs have a two-hour
job limit and one-hour per-file limit so benchmarks can finish and measure any
overrun of the normal PR budget.

The initial [full CI audit](https://github.com/p4lang/p4c/actions/runs/40137227093)
completed 970 translation units in **48m49s**, with no analysis failures.
Preparation took up to 10m45s and analysis jobs up to 37m57s. Generated IR alone
took 36m06s. The job and per-file budgets accommodate that measured workload.
"No analysis failures" means no crashes, timeouts or parsing errors: full audits
still report existing warnings without promoting them to errors. PR mode fails
on warnings reported at changed lines.

### Full audits and local commits

```sh
# Report all findings in the configured source set, without changed-line filtering.
uv run cmake --build build-tidy --target clang-tidy
# Analyze staged changes; partial staging of C++ files is rejected.
uv run python tools/run-clang-tidy.py --build-dir build-tidy --staged
# Measure individual check costs and save per-file results.
uv run python tools/run-clang-tidy.py --build-dir build-tidy --profile --report tidy.json
```

Full audits report existing warnings; parsing errors and timeouts still fail.
`clang-tidy-apply-fix-its` applies fixes serially to avoid concurrent edits of
shared headers. The commit hook checks only formatting so commits do not require
a configured build. The old `add_cpplint_files` CMake helper is a compatibility
alias for extension source registration; the cpplint executable, configuration,
and build targets have been removed.

## check-git-submodules
A helper script that checks that each of the submodules in the repository remains on the branch that it is tracking. The intention is to avoid accidentally switching to a non-stable or outdated branch for a particular dependency.

To check whether all heads are on the correct branch run
```
./check-git-submodules.sh
```
