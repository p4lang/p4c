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

* the P4_14 (P4 v1.0) language is described in the [P4 spec](http://p4.org/wp-content/uploads/2015/04/p4-latest.pdf)

* the [P4_16 draft language](http://p4.org/wp-content/uploads/2016/12/P4_16-prerelease-Dec_16.pdf)
  specification is still under revision.

* the core design of the compiler intermediate representation (IR) and
  the visitor patterns are briefly described in [IR](IR.md)

* The [migration guide](migration-guide.pptx) describes how P4_14 (v1.0)
  programs are translated into P4_16 programs
