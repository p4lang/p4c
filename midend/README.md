<!--!
\page midend Midend                                                                      
-->
<!-- 
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.

Refer to the full page here: [Midend](https://p4lang.github.io/p4c/midend.html)
-->
<!--!
\internal
-->
# Midend

This folder contains passes which may be useful for implementing various mid-ends.
These are linked as part of the front-end library.
These passes are not used in the front-end, but are still largely architecture-independent.

<!--!
\endinternal
-->
The mid-end stage applies further optimizations to the IR of the P4 program. It's architecture-independent, it is driven by policies specific to the target architecture. The mid-end uses the same base IR as the front-end and includes the following passes:

## Optimization Passes:
  - **Creating Actions/Tables from Statements and Actions:** Converts high-level constructs into more granular actions and tables.
  - **Eliminating Tuple and Enum Types:** Simplifies complex data types into more manageable forms.
  - **Predicating Code:** Converts conditional statements (e.g., if statements) into predicate expressions (e.g., using the ternary ?: operator).

## Pass Library:
The mid-end uses optimization passes to refine the IR, enhancing efficiency for the back-end.
