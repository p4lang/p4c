# portable_common

This directory contains reusable components common to both the `psa_switch` and `pna_nic` backends.



|      **File Name**          |      **Description**     |
|-----------------------------|--------------------------|
| **midend.h, midend.cpp**    | Defines the common mid-end processing of both the `psa_switch` and `pna_nic` backends.    |
| **options.h, options.cpp**  | Defines the common command-line options of both the `psa_switch` and `pna_nic` backends.   |
| **portable.h, portable.cpp**| Defines common functionalities that generate representations of P4 programs.   |
| **portableProgramStructure.h, portableProgramStructure.cpp** | Defines and implements the common program structure of both the `psa_switch` and `pna_nic` backends. _These files are located in the `backends/common` directory._ |
