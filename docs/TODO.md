# Open Issues in the P4 compiler toolchain

This document describes the major work items planned or undergoing.
For specific bugs use the issue tracking for the project in github.

## Code quality

Many more test programs have to be written for all front-ends and back-ends.

Write end-to-end tests for eBPF.

Many more test inputs (STF files) have to be written.

## Language extensions

Add to P4 various desired extensions

* Add header_union datatypes
* Add table initializers (for constant tables at least)
* Calls with arguments in any order, by specifying parameter names
* Namespaces
* Inferred deparsers for P4 v1.2 programs
* Functions
* Logging constructs
* Action sets (an action that can be composed by combining many simpler actions)

## Completeness

* V1 to V1.2 translation
  * handle varbit fields
  * parser exceptions?
  * truncate() primitive action
  * "payload" field-list element

* Define and implement "standard" architectural model(s).

* Implement BMv2 back-end(s) for the standard architecture model(s).

* Write many mid-end lowering passes.
  * Parser inlining
  * Parser loop unrolling
  * Def-use analysis
  * Detect potential uses of undefined values
  * Implement arithmetic on strange word sizes in terms of platform-available arithmetic
  * Code motion between actions
  * Field alignment manipulations
  * Split/merge fields
  * Cross-block optimizations
  * enum/error representation

* Refactor BMv2 back-end to use BMv2 black-boxes instead of primitive
  actions

## Documentation

The IR has to be documented.

The compiler structure needs more documentation.

Document the eBPF back-end.

## Miscelaneous issues

* Separate clearly IR data from C++ IR manipulation.
* IR serialization to/from text files.

## External tools

* Make BMv2 support the full P4 v1.2 language
  * required black-boxes
  * enums/errors
  * set correct parser error codes
  * casts
