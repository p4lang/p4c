<!--!
\page p4test_backend P4test Backend                                                               
-->
<!-- 
Documentation Inclusion:
This README is integrated as a standalone page in the P4 compiler documentation.

Refer to the full page here: [P4test Backend](https://p4lang.github.io/p4c/p4test_backend.html)
-->
<!--!
\internal
-->
# P4test Backend
<!--!
\endinternal
-->

<!--!
[TOC]
-->
The P4test Backend is a tool designed for testing and debugging P4
programs. It supports both the P4_14 and P4_16 standards and can
translate P4 code from one version to another. Additionally, it
provides a syntax checker for P4 code, enabling the verification of 
the correctness of your P4 programs. It supports both the P4_14 and 
P4_16 standards and can translate P4 code from one version to another.

## Auto-translate P4_14 source to P4_16 source:
You can automatically translate a P4_14 program to a P4_16 program
using the following command:
```bash
p4test --std p4-14 my-p4-14-prog.p4 --pp auto-translated-p4-16-prog.p4
```

This command takes the P4_14 program (my-p4-14-prog.p4) and generates
a corresponding P4_16 program (auto-translated-p4-16-prog.p4).


## Check syntax of P4_16 or P4_14 source code
The P4test Backend can check the syntax of P4 programs without being
restricted by any specific compiler back end. This is useful for
ensuring that your P4 code is syntactically correct.

For P4_16 code: 
```bash
p4test my-p4-16-prog.p4
```

For P4_14 code: 
```bash
p4test --std p4-14 my-p4-14-prog.p4
```

These commands will output error and/or warning messages if there 
are any issues with the syntax of your P4 code, enabling you to
verify the correctness of your P4 programs, due to that it is 
possible to verify the correctness of your P4 programs.
