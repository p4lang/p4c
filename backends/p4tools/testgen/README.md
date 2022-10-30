# P4Testgen - Generating Tests for P4 Targets

The P4Testgen tool uses symbolic execution to automatically generate input-output tests for a given P4 program. P4Testgen attempts to generate a set of tests of all reachable paths within the program. Each test consists of an input packet, the control-plane commands to populate match-action tables, and a predicate on the output packet. Each generated test also has a human-readable trace that indicates the path through the program being exercised by that test.

## Directory Structure

```
testgen
 ├─ cmake                      ── common CMake modules
 ├─ core                       ── C++ source: testgen symbolic executor core
 │   ├─ exploration_strategy   ── exploration strategies for the testgen symbolic executor
 │   └─ small_step             ── code for the testgen symbolic generator and test case generation
 ├─ lib                        ── C++ source: testgen library files
 ├─ targets
 │   └─ bmv2                   ── the extension for the behavioral model [simple switch](https://github.com/p4lang/behavioral-model/blob/main/targets/README.md#simple_switch)
 └─ test                       ── Unit tests
```

## Usage
The main binary can be found in `build/p4check`.

To generate tests for a particular target and P4 architecture, run `p4check testgen –target [TARGET] –arch [ARCH] –max-tests 10 –out-dir [OUT] prog.p4`
`p4check` is an umbrella binary which delegates execution to all tools of the p4-tools repository.
`testgen` specifies that the p4testgen tool should be used. In the future, other tools may be supported, for example random program generation or translation validators.
These are the current usage flags:

```
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
--exploration-strategy     Selects a specific exploration strategy for test generation. Options are: randomAccessStack, linearEnumeration, maxCoverage. Defaults to incrementalStack.
--pop-level                This is the fraction of unexploredBranches we select on randomAccessStack. Defaults to 3.
--linear-enumeration       Max bound for LinearEnumeration strategy. Defaults to 2. **Experimental feature**.
--saddle-point             To be used with randomAccessMaxCoverage. Specifies when to randomly explore the map after a saddle point in the coverage of the test generation.
```

Once P4Testgen has generated tests, the tests can be executed by either the P4Runtime or STF test back ends.

### Running ctest
Once compiled, run ctest in the build directory:
```
ctest -V -R testgen
```

### Additional command line parameters:
The ```--top4 ``` option in combination with ```--dump``` can be used to dump the individual compiler pass. For example, ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4   ".*" prog.p4``` will dump all the intermediate passes that are used in the dump folder. Whereas ```p4testgen --target bmv2 --std p4-16 --arch v1model --dump dmp --top4 "FrontEnd.*Side" prog.p4``` will only dump the side-effect ordering pass.
