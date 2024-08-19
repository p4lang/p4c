<!-- 
Documentation Inclusion:
This README is integrated as a subsection of the "Behavioral Model Backend" page in the P4 compiler documentation.

Refer to the specific section here: [BMv2 "pna_nic" Backend - Subsection](https://p4lang.github.io/p4c/behavioral_model_backend.html#bmv2-pna_nic-backend)
-->

# BMv2 "pna_nic" Backend

The [`backends/bmv2/pna_nic` directory](https://github.com/p4lang/p4c/tree/main/backends/bmv2/pna_nic) contains components specific to the BMv2's PNA NIC (Portable NIC Architecture) backend in the P4C compiler. The files in this folder depend on each other, on the files in the `bmv2/common` and  `portable_common` directories. Most of the classes are inherited from the classes in the `portable_common` directory.

Output Binary: `p4c-bm2-pna`

#### pnaProgramStructure.h, pnaProgramStructure.cpp

Defines and implements the program structure (metadata, parsers, controls, and deparsers) and parsing logic specific to the BMv2's PNA NIC backend.

##### midend.h, midend.cpp

Defines the mid-end processing of the PNA NIC compiler. Performs various transformations and optimizations on the program's Intermediate Representation (IR).

##### options.h, options.cpp

Manages the command-line options for the PNA NIC compiler.

##### pnaNic.h, pnaNic.cpp

Provides backend implementation to the BMv2's PNA NIC compiler.

##### main.cpp

Sets up compilation environment, integrates various components, and executes the PNA NIC compiler.

##### version.h.cmake

Defines macros containing version information for the PNA NIC compiler.
