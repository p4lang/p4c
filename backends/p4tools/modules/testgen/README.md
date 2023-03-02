# P4Testgen - Generating Tests for P4 Targets

P4Testgen uses symbolic execution to automatically generate input-output tests for a given P4 program. P4Testgen attempts to generate a set of tests of all reachable paths within the program. Each test consists of an input packet, the control-plane commands to populate match-action tables, and a predicate on the output packet. Each generated test also has a human-readable trace that indicates the path through the program being exercised by that test.

## Directory Structure

```
testgen
 ├─ cmake                      ── common CMake modules
 ├─ core                       ── C++ source: testgen symbolic executor core
 │   ├─ symbolic_executor   ── exploration strategies for the testgen symbolic executor
 │   └─ small_step             ── code for the testgen symbolic generator and test case generation
 ├─ lib                        ── C++ source: testgen library files
 ├─ targets
 │   ├─ bmv2                   ── the extension for the behavioral model [simple switch](https://github.com/p4lang/behavioral-model/blob/main/targets/README.md#simple_switch)
 │   └─ ebpf                   ── the extension for the [P4-to-eBPF kernel target](https://github.com/p4lang/p4c/tree/main/backends/ebpf).
 └─ test                       ── Unit tests
```

## Usage
The main binary can be found in `build/p4testgen`.

To generate 10 tests for a particular target and P4 architecture, run `p4testgen –target [TARGET] –arch [ARCH] –max-tests 10 –out-dir [OUT] prog.p4`
`p4testgen` specifies that P4Testgen should be used. In the future, other tools may be supported, for example random program generation or translation validators.
These are the current usage flags:

```
./p4testgen: Generate packet tests for a P4 program.
--help                                       Shows this help message and exits
--version                                    Prints version information and exits
--seed seed                                  Provides a randomization seed
-I path                                      Adds the given path to the preprocessor include path
--Wwarn diagnostic                           Report a warning for a compiler diagnostic, or treat all warnings as warnings (the default) if no diagnostic is specified.
-D arg=value                                 Defines a preprocessor symbol
-U arg                                       Undefines a preprocessor symbol
-E                                           Preprocess only. Prints preprocessed program on stdout.
--nocpp                                      Skips the preprocessor; assumes the input file is already preprocessed.
--std {p4-14|p4-16}                          Specifies source language version.
-T loglevel                                  Adjusts logging level per file.
--target target                              Specifies the device targeted by the program.
--arch arch                                  Specifies the architecture targeted by the program.
--top4 pass1[,pass2]                         Dump the P4 representation after
                                             passes whose name contains one of `passX' substrings.
                                             When '-v' is used this will include the compiler IR.
--dump folder                                Folder where P4 programs are dumped.
-v                                           Increase verbosity level (can be repeated)
--strict                                     Fail on unimplemented features instead of trying the next branch.
--input-packet-only                          Only produce the input packet for each test.
--max-tests maxTests                         Sets the maximum number of tests to be generated [default: 1]. Setting the value to 0 will generate tests until no more paths can be found.
--stop-metric stopMetric                     Stops generating tests when a particular metric is satisifed. Currently supported options are:
                                             "MAX_STATEMENT_COVERAGE".
--packet-size-range packetSizeRange          Specify the possible range of the input packet size in bits. The format is [min]:[max]. The default values are "0:72000". The maximum is set to jumbo frame size (9000 bytes).
--out-dir outputDir                          The output directory for the generated tests. The directory will be created, if it does not exist.
--test-backend testBackend                   Select the test back end. P4Testgen will produce tests that correspond to the input format of this test back end.
--input-branches selectedBranches            List of the selected branches which should be chosen for selection.
--track-branches                             Track the branches that are chosen in the symbolic executor. This can be used for deterministic replay.
--with-output-packet                         Produced tests must have an output packet.
--exploration-strategy SymbolicExecutor   Selects a specific exploration strategy for test generation. Options are: DEPTH_FIRST, RANDOM_BACKTRACK, BOUNDED_RANDOM_BACKTRACK, GREEDY_STATEMENT_SEARCH, DETERMINISTIC_STATEMENT_SEARCH, RANDOM_STATEMENT_SEARCH, and LINEAR_ENUMERATION. Defaults to DEPTH_FIRST.
--pop-level popLevel                         Sets the fraction for multiPop exploration; default is 3 when meaningful strategy is activated.
--linear-enumeration linearEnumeration       Max bound for vector size in LINEAR_ENUMERATION; defaults to 2.
--saddle-point saddlePoint                   Threshold to invoke multiPop on RANDOM_STATEMENT_SEARCH.
--print-traces                               Print the associated traces and test information for each generated test.
--print-steps                                Print the representation of each program node while the stepper steps through the program.
--print-coverage                             Print detailed statement coverage statistics the interpreter collects while stepping through the program.
--print-performance-report                   Print timing report summary at the end of the program.
--dcg DCG                                    Build a DCG for input graph. This control flow graph directed cyclic graph can be used
                                                     for statement reachability analysis.
--pattern pattern                            List of the selected branches which should be chosen for selection.
```

Once P4Testgen has generated tests, the tests can be executed by either the P4Runtime or STF test back ends.

### Running ctest
Once compiled, run ctest in the build directory:
```
ctest -V -R testgen
```

### Additional command line parameters:
The ```--top4 ``` option in combination with ```--dump``` can be used to dump the individual compiler pass. For example, ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4   ".*" prog.p4``` will dump all the intermediate passes that are used in the dump folder. Whereas ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4 "FrontEnd.*Side" prog.p4``` will only dump the side-effect ordering pass.
