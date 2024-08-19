<!-- 
Documentation Inclusion:
This README is integrated as a subsection of the "Behavioral Model Backend" page in the P4 compiler documentation.

Refer to the specific section here: [portable_common - Subsection](https://p4lang.github.io/p4c/behavioral_model_backend.html#portable_common)
-->
# portable_common

This directory contains reusable components common to both the `psa_switch` and `pna_nic` backends.

### midend.h, midend.cpp

Defines the common mid-end processing of both the `psa_switch` and `pna_nic` backends.

### options.h, options.cpp

Defines the common command-line options of both the `psa_switch` and `pna_nic` backends.

### portable.h, portable.cpp

Defines common functionalities that generate representations of P4 programs.

The files `portableProgramStructure.h` and `portableProgramStructure.cpp` are in the `backends/common` directory.

### portableProgramStructure.h, portableProgramStructure.cpp

Defines and implements the common program structure of both the `psa_switch` and `pna_nic` backends.
