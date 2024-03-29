# Overview

The P4C compiler is a compiler infrastructure for the P4 compiler designed with the following goals:

* Support current and future versions of P4
* Support multiple back-ends
  * Generate code for ASICs, NICs, FPGAs, software switches and other targets
* Provide support for other tools (debuggers, IDEs, control-plane, etc.)
* Open-source front-end
* Extensible architecture (easy to add new passes and optimizations)
* Use modern compiler techniques (immutable IR, visitor patterns, strong type checking, etc.)
* Comprehensive testing


## Additional documentation

* The documentation for P4_16 and P4Runtime is available [here](https://p4.org/specs/)

* the core design of the compiler intermediate representation (IR) and
  the visitor patterns are briefly described in [IR](../IR.md)

* The [migration guide](../migration-guide.pptx) describes how P4_14 (v1.0)
  programs are translated into P4_16 programs
