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

### Partially constant parser selects

Typed constant folding specializes parser `select` expressions whose tuple contains
both compile-time constants and runtime values. This lets parameterized parsers
select protocol families at instantiation time without carrying constant dimensions
into the target's parser lookup key. For example:

```p4
select(16w1, hdr.protocol) {
    (2, 17): parse_disabled;
    (1, 6): parse_tcp;
    default: accept;
}
```

becomes `select(hdr.protocol)` with `6: parse_tcp` and `default: accept`.
The impossible `parse_disabled` edge is removed. The same transformation applies to
constant dimensions matched by masks, inclusive ranges, Booleans, enums, and constant
tuples. Remaining cases retain their original order, including overlapping cases.

The transformation runs through the shared constant folder, including after parser
constructor specialization and during midend lowering. Newly constructed tuple types
are checked by subsequent type checking; the constant-folding pass manager clears the
type map after rewriting. Parser simplification can then remove unreachable states,
and target lowering receives the reduced select key. Source export preserves a
singleton outer tuple with braces when projection leaves a nested tuple, so the
exported P4 can be parsed and type checked again.

A dimension is retained if any of its keysets cannot be compared at compile time.
An opaque keyset such as a runtime value set retains the entire select's tuple schema
and control-plane API. Partially constant nested tuples can be specialized further
after select-list flattening. Runtime select expressions remain in evaluation order,
even when every case is removed, preserving side effects and `error.NoMatch` behavior.
Selects with side effects wait until side-effect ordering has lifted calls into
statements, so early parser chain collapsing cannot discard their evaluation.
This optimization introduces no annotations, options, or new target requirements.

The `partial-parser-select` compiler regressions check the exact `error.NoMatch`
lowering. The BMv2 packet test covers supported matching paths and observes every
possible output port. It does not assert implicit NoMatch behavior: some BMv2
runtimes finish an unmatched select without setting `parser_error`. Compiler
references and the `EmptySelectPreservesNoMatch` GTest cover that path independently.
