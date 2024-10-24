# Overview of bf-p4c

## Setup

This repo contains the tofino backend for p4c v1.2 compiler.  It contains
*just* the backend and can only be built within the p4c build infrastructure
in git@github.com:p4lang/p4c.  To use this repo:

    git clone git@github.com:barefootnetworks/p4c-extension-tofino.git
    cd p4c-extension-tofino
    ./bootstrap.sh

This will check out the p4lang/p4c repository, move the
p4c-extension-tofino to `p4c/extensions/bfn`, install the
necessary dependences, and build the project.

## Dependencies

- Tofino assembler and Harlyn model for testing the output of
  the Tofino backend.  The test scripts will look for sibling repos
  (in preference to installed versions), so the easiest is to
  check out these repos in parent of the `p4c` folder and build them
  there:

```
git clone git@github.com:barefootnetworks/tofino-asm.git
git clone git@github.com:barefootnetworks/model.git
cd tofino-asm
make all
cd ..
cd model
./autogen.sh
./configure
make
```

- It may be desirable to checkout the "cdodd-simple_harness" branch of
  the model to get the latest code for the test harness we're using.

- The Harlyn model requires additional dependences; please refer to
  its own documentation:
  https://barefootnetworks.atlassian.net/wiki/pages/viewpage.action?pageId=17006668.
  Only the dependences required by the "model" are needed (i.e., we
  don't use the driver in the compiler tests).

- The tests depend on GNU utilities. On macOS, these can be installed with
  `brew install coreutils`.

## Tofino mid-end and back-end design

The mid-end and back-end of the compiler are where all the Tofino-specific
work is done.  To some extenet the boundary between mid-end and back-end
is arbitrary, but the most logical place to put it is at the point where
we transform the IR from frontend P4-oriented form (generally, a global scope
containing a bunch of named symbols of various types) to Tofino-oriented
form (a Pipe object containing ingress and egress parsers, deparsers, and tables).

### Mid-end

There are a variety of transformations we may want to do on the IR
when it is still in P4 oriented form.  These could be considered
non-tofino-specific transforms, since they operate at the frontend IR
level, but are generally directed by target-specific considerations.
Some could also be implemented to operate on the backend IR form.

* Sequential action analysis.  Tofino actions happen in parallel, so
  to ensure proper sequential behavior, we need to analyze actions and
  substitute intermediate temporaries (essentially, copt propagation)
  to ensure that each of the primitives within an action can be executed
  in parallel.

* Field packing and unpacking.  Tofino can only deal with 8, 16, and 32
  bit quantities, so fields larger than 32 bits must be split.  The parser
  operates on bytes, so fields in headers that need to be parsed/deparsed
  that aren't on byte boundaries need to be reorganized so as to be
  byte aligned.  This may necessitate introducing additional metadata.

* POV allocation.  Tofino requires POV bits stored as additional metadata
  for the deparser to control deparsing.  These bit should also be used
  for validity checking of headers in tables.

* Bridged metadata.  We need to introduce a bridged metadata fake header
  that is deparsed from the ingress and parser in egress to carry metadat
  information between ingress and egress.

### Back-end

The backend is where most of the resource allocation decisions take
place.  Resource allocation passes should take extra ‘hints’ inputs to
drive decisions based on previous failed attempts that triggered
backtracking.  This is a potential exponential blowup, so care needs
to be taken to avoid attempts that are probably unprofitable -- tuning
will be needed.

To the extent that it does not complicate the code too much, it should
be parameterized based on architecture details.  The intent is
that the code should be reusable for multiple architecture families.

The code here is organized into `parde` (parser-deparser), `mau`
(match-action table), and `phv` (phv allocation and management) directories.
To some extent these are independent, but there are interdependecies.

#### IR

There are a variety of Tofino-specific IR classes used to hold information
needed to track tofino resource use and manage the allocation of those
resources.

###### `IR::BFN::Pipe`

An abstraction of a single Tofino pipeline, this object is basically
just a container for other (Parser, Deparser, and MAU) specific objects.
This becomes the root object of the IR tree after the mid-end.

Because most visitors only care about part of the Pipe object (eg,
just the MAU, or just the Parser), we define special visitor bases
`MauInspector`, `MauModifier`, `MauTransform`, `PardeInspector`,
`PardeModifier`, and `PardeTransform` that visit just those parts of the
tree of interest to Mau or Parde.  Other parts of the tree are skipped.

###### `IR::BFN::Unit`

Abstract base class for parts of the pipe that access the PHV -- tables,
parser states, and the deparser.  The `stage()` function returns an integer
associated with the order of the unit's execution -- the actual stage for
tables, -1 for parser states (they come before stage 0), and a large
number for the deparser (after all stages).

###### `IR::BFN::Parser`
###### `IR::BFN::Deparser`

###### `IR::MAU::Table`

The mau code is largely organized around the `IR::MAU::Table` ir class, which
represents a Tofino-specific logical table.  Such a table has an (optional)
gateway, with an expression, condition, and (optional) payload action, an
(optional) match table, and zero or more attached tables (indirect, action
data, selector, meter, counter, ...).  There is a next table map that
contains references to sequences of other tables to run based on the
gateway result or action run.

- `gateway_expr`  boolean expression that the gateway tests, or `nullptr`
  if there is no gateway
- `gateway_payload`  action to run if the match-action table does not run
- `gateway_cond`  if `eval(gateway_expr) == gateway_cond` then the
  `match_table` should run.  If there is no `gateway_expr` or no
  `match_table`, this is ignored.
- `match_table`  match table to run iff `gateway_expr == nullptr` or
  `eval(gateway_expr) == gateway_cond`
- `actions`  list of action functions that might be in the table.
  **TBD:** probably need to differentiate default-only vs non-default-only vs
  other actions somehow?
- `attached`  list of attached support tables -- ternary indirection,
  action data, selection, meter, counter, stateful alu...
- `next`  map from gateway conditions and actions to control dependent table
  sequences.  Keys are cstrings that may be action names or keywords.
  - `true` -- next to run iff `eval(gateway_expr) == true`.  Not allowed if
    there is a `match_table` and `gateway_cond == true`, since then the match
    table result applies.
  - `false` -- next to run iff `eval(gateway_expr) == false`.  Not allowed if
    there is a `match_table` and `gateway_cond == false`, since then the match
    table result applies.
  - *action*  -- next to run if the `match_table` runs this action.
  - `$miss`  -- next to run if the `match_table` runs and misses.  This will
    override if the action also specifies a next.
  - `default`  -- next to run if the `match_table` runs and nothing else
    applies.

`IR::MAU::Table` objects are initially created by the mid-end
corresponding to each gateway and match table in the P4 program.
These tables will then be manipulated (split, combined, and reordered)
by various passes to get them into a form where they can be fit into the
Tofino pipeline, then allocated to specific stages/logical ids/resources
within the pipeline.

###### `IR::MAU::TableSeq`

A sequence of logical tables to run in order.  Each table will run,
followed by any control dependent tables, followed by the next
table in the sequence.  By definition, tables in a `TableSeq` are
control-independent, but may be data-dependent.  The `TableSeq` also
contains a bit-matrix that tracks data dependencies between tables in
the sequence including their control-depedent tables.  Tables that are
not data dependent may be reordered.

#### Passes

The first step in the Tofino backend is converting the P4-level IR into the
needed forms for the backend.  In most cases, this will be a fairly simple
wrapper around some P4-level IR objects, organizing them into the rough form
that tofino requires.

##### parde

Parser and deparser processing starts with extracting the state machine
graph from the P4-level IR.  Since the IR (currently) does not support
loops, but the state machine may involve loops, this will require using
a representation that breaks the loops.  We could extend the IR to
allow loops (would require extending the Visitor base classes to deal
with loops rather than just detecting them and throwing an error), but
that may not be necessary.  A series of transformation passes can then
optimize the parser:

* Loop unrolling -- states that have backreferences (loops) can be unrolled by
  cloning the backreferenced tree, limited by the maximum header stack size
  involved

* Parser state splitting -- states that do too much (write too many
  output bytes) need to be split into multiple states.  States could also
  be split into minimal states that each do a single thing that will later
  bre recombined.

* Parser state combining -- small consecutive states can be combined.

It may be that simply splitting states into the smallest possible
fragments and then recombining them works well for general optimization.
Alternately, other things could be tried.  The important thing is
flexibility in the representation, to permit experimentation.

Deparser handling in v1.1 is a matter of extracting the deparser from the
parser state machine and blackboxes that do deparser-relevant processing
(checksum units, learning filters, ...).  Its not clear whether this is
best done from the P4-level parser IR, or from some later state of the
parser IR (after some backend transformations).

##### mau

Mau processing begins by converting the P4 IR into a pair of
`IR::MAU::TableSeq` objects (one for ingress, and one for egress --
this is the 'mid-end'), which is prototyped in `extract_maupipe.cpp`.
Then the code can reorganized by various optimization passes that will
be needed:

- Gateway analysis and splitting -- gateway tables that are too complex
  for tofino must be split int multiple gateways.

- Gateway merging -- simple consecutive gateways can be combined into
  a single gateway

- Gateway duplication -- gateways that have multiple tables dependent
  on their result can be duplicated, with each duplicated gateway having
  fewer dependent tables.  This allows more flexibility for table scehduling
  and may only be done if table placement fails and backtracks.  A
  prototype implemention exists in `split_gateways.cpp`.

- Table analysis -- figure out what resources a logical table will need
  (at least number of memories, ixbar space, other units) for table
  placement to refer to.  Decide on width, exact match groups, ways, putting
  immediate data into overhead vs action data table etc.  Another place
  that we may backtrack to if table placement fails.  A partial prototype
  exists in `table_layout.cpp`

- Table insertion -- if later passes decide they need to insert new tables,
  we need a 'pass' to backtrack to and do that.  First time through this
  pass will do nothing, then other passes may trigger a 'new table needed'
  backtrack that backtracks to this pass to insert the table.

- Table reorg -- any optimizations that involve splitting or combining
  tables may occur here.  For example, actions that are too complex for
  tofino could be implemented by splitting off part of the action into a
  followon table.

- Table dependency analysis -- find match, action, and write(reverse)
  dependencies between tables

Once the these initial passes are complete, we can do the main table
placement analysis.  This pass will map logical tables to specific
stages and logical ids.  Gateways with no match table may be combined with
match tables with no gateway as part of this process.  Table sequences may
be reordered if dependencies allow to better fit.  Tables that do not fit
within a stage will be split into multiple tables.  A prototype implementation
exists in `table_placement.cpp`, but not properly support backtracking.

After table placement, tables will be allocated to specific resources
in their stage (some of this may be concurrent with placement, to the
extent that it affects placement).  Unused resources (memories) may be
added to tables to allow them to expand to larger than minimum required
size.

Flow analysis of metadata and header field use in tables then determines
which metadata fields are independent, as well as identifying all the
phv use constraints on fields based on how they are use in tables.

Once final PHV allocation has been done, tables may need to be modified to
work with the allocation.

- transform action statements that operate on fields that have been split
  by phv allocation into mulitple statements.

- allocate action data to the action bus based on which PHV registers need
  access to it.

a single pass over the final IR should suffice for generating asm code.

##### phv

PHV allocation and management interacts heavily with the other parts of the
compiler.  The basic design is to have an initial PHV pass that gathers
basic information on all header and metadata fields in the program, then passes
in mau and parde can augment this information with additional constraints as
the run.

Once most of the allocation decisions for other passes have been made, PHV
allocation will assign physical PHV slots for each field, possibly splitting
fields.  When this fails, PHV allocation needs to provide information about
why it is failing (running out of a particular size, or group, or ?) that
can be used by passes we backtrack to to modify their uses.

We can build an interference graph for all fields, allowing use to experiment
with graph coloring algorithms for PHV allocation, as well as simpler
greedy allocation.

Inspector passes to find constraints could logically be either part of
the component they are anaylzing (parde or mau) or part of phv allocation.

# Driver

Driver is a program that interacts with the user and drives the whole
compilation process.

Driver consists of two Python 3 scripts `driver.py` and `barefoot.py` 
(see `barefoot.BarefootBackend` class).
From inside the compiler, the `bf-p4c-options` handles the arguments and
their additional parsing.

__Developer environment variables__:
* `P4C_BUILD_TYPE` - if set to `DEVELOPER` then the driver will parse developer 
arguments.
* `P4C_BIN_DIR` - Directory containing P4C binaries (p4c-barefoor, bfas...).
* `P4C_CFG_PATH` - Schemas directory.

Other environment variables and all command line options are documented
in the [user guide wiki](https://wiki.ith.intel.com/display/BXDCOMPILER/Getting+started)

## driver.py

This script is part of the public frontend and serves as the base driver.
It is being invoked by `barefoot.py` with setup argument parser and commands.
This script sets up the output program structure (output folder) and
executes all setup processes.

## barefoot.py

Driver for bf-p4c. It sets up all the command line options and commands
for specific compilation processes (such as preprocessor invocation...).

## Driver testing

There are scripts to test driver's correctness - `test_p4c_driver.py`,
which runs the tests and `p4c_driver_tests.py`, which contains the tests.

To add new tests for driver, one has to edit the `test_matrix` dictionary
in `p4c_driver_tests.py`. Each entry in this dictionary is a test. To add
a new test a new key has to be created and its value has to be a tuple
with following parameters (in order listed):
0. List of compiler arguments.
1. String with expected xfail message or None if fail is not expected.
2. List of files that the driver should have or have not created after 
the test being run. The file has to be specified by a relative path and if
the path includes `!` as the first character, then this checks that the
file does not exist. E. g. if `/pipe/foo` was used, then tests will
check that file `foo` exists in a folder `pipe` after compilation of the test.
Using `!/pipe/foo` would check that the file DOES NOT exist.
3. Reference source json (`source.json`) file or None.
4. Reference configuration file or None.
5. Expected string in STDOUT or None.

# Compiler's output

General documentation for the compiler output can be found on the wiki:
* [Synopsis of Compiler Inputs and Outputs](https://wiki.ith.intel.com/pages/viewpage.action?pageId=1981604254#Gettingstarted-SynopsisofCompilerInputsandOutputs)
* [The Manifest File: Information on the Generated Files](https://wiki.ith.intel.com/pages/viewpage.action?pageId=1981604254#Gettingstarted-TheManifestFile:InformationontheGeneratedFiles)
* [Compiler output for debugging](https://wiki.ith.intel.com/display/BXDCOMPILER/Appendix+F_+Compiler+Output+for+Debugging)

Produced JSON files have certain schemas, which have to adhere to [schema policies](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/README.md#schema-versioning-policy). When
changing schemas, these changes have to be approved by the _schema review board_ 
this is because these schemas are often used by mutliple projects (p4c, p4i,
drivers...) and since this is always produced by p4c, the p4c approval is 
always needed. More about the review process can be found [here](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/README.md#contributions-and-change-process).

| __File__        | __Schema__          | __Consumers__        | __Schema__ |
|-----------------|---------------------|----------------------|------------|
| context.json    | Context_schema      | Drivers, Model, P4I  | [context_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/context_schema.py) |
| bfrt.json       | Bfrt_schema         | Drivers              | [bfrt_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/bfrt_schema.py) |
| manifest.json   | Manifest_schema     | Drivers, P4I         | [manifest_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/manifest_schema.py) |
| resources.json  | Resources_schema    | P4I                  | [resources_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/resources_schema.py) |
| mau.json        | Mau_schema          | P4I                  | [mau_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/mau_schema.py) |
| phv.json        | Phv_schema          | P4I                  | [phv_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/phv_schema.py) |
| dep.json        | Table_Graph_schema  | P4I                  | [table_graph_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/table_graph_schema.py) |
| power.json      | Power_schema        | P4I                  | [power_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/power_schema.py) |
| source.json     | Source_info_schema  | P4I                  | [source_info_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/source_info_schema.py) |
| metrics.json    | Metrics_schema      | P4C                  | [metrics_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/metrics_schema.py) |
| events.json     | Event_log_schema    | P4I                  | [event_log_schema.py](https://github.com/intel-restricted/networking.switching.barefoot.compiler-interfaces/blob/master/schemas/event_log_schema.py) |


Assembler source code (`*.bfa`) is a structured file using YAML syntax described in `SYNTAX.yaml` and is extensible for multiple architectures (Tofino, Tofino2...).
