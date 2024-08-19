<!--!
\page frontend Frontend                                                                      
-->
<!-- 
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.

Refer to the full page here: [Frontend](https://p4lang.github.io/p4c/frontend.html)
-->
<!--!
\internal
-->
# Frontend 
<!--!
\endinternal
-->
The front end of the P4 compiler translates P4 source code into the P4 Intermediate Representation (P4C-IR), performs type-checking, and ensures that the P4 program complies with the P4 specification. The front end also applies architecture-independent optimizations such as Strength Reduction, Constant Folding, Inlining, or Dead-Code Elimination. This is a non-exhaustive list of passes:

- **Program Parsing:** Converts the P4 source code into an Abstract Syntax Tree (AST).
- **Validation:** Checks the program against P4 language rules and constraints to ensure it is valid.
- **Name Resolution:** Resolves all identifiers, such as variables and functions, to their declarations.
- **Type Checking/Inference:** Uses the Hindley-Milner type inference algorithm to ensure that all types are correctly assigned and compatible.
- **Making Semantics Explicit:** Ensures the program's semantics, such as the order of side effects, are clearly defined and explicit.
## Optimization Tasks
The front-end also performs some optimization-related tasks, including:
- **Inlining:** Replaces function calls with the function body to reduce overhead.
- **Compile-Time Evaluation and Specialization:** Evaluates expressions and specializes code at compile time for efficiency.
- **Conversion to P4 Source:** Converts the AST back to P4 source code if needed.
- **Deparser Inference (for P4_14 programs):** Automatically infers deparser logic for P4_14 programs.

After completing these passes, the front-end generates the control-plane API, which is essential for the interaction between the control plane and the data plane.
