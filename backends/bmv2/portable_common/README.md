# `portable_common`

This directory contains reusable components common to both the `psa_switch` and `pna_nic` backends.

##### midend.h, midend.cpp

Defines the common mid-end processing of both the `psa_switch` and `pna_nic` backends.

##### options.h, options.cpp

Defines the common command-line options of both the `psa_switch` and `pna_nic` backends.

##### portable.h, portable.cpp

Defines common functionalities that generate representations of P4 programs.

----

The files `portableProgramStructure.h` and `portableProgramStructure.cpp` are in the `backends/common` directory.

#### portableProgramStructure.h, portableProgramStructure.cpp

Defines and implements the common program structure of both the `psa_switch` and `pna_nic` backends.