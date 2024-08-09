# BMv2 "pna_nic" Backend

This directory contains components specific to the BMv2's PNA NIC (Portable NIC Architecture) backend in the P4C compiler. The files in this folder depend on each other, on the files in the `bmv2/common` and  `portable_common` directories. Most of the classes are inherited from the classes in the `portable_common` directory.

Output Binary: `p4c-bm2-pna`

| File(s)                      | Description |
|------------------------------|-------------|
| `pnaProgramStructure.h`, `pnaProgramStructure.cpp` | Defines and implements the program structure (metadata, parsers, controls, and deparsers) and parsing logic specific to the BMv2's PNA NIC backend. |
| `midend.h`, `midend.cpp`     | Defines the mid-end processing of the PNA NIC compiler. Performs various transformations and optimizations on the program's Intermediate Representation (IR). |
| `options.h`, `options.cpp`   | Manages the command-line options for the PNA NIC compiler. |
| `pnaNic.h`, `pnaNic.cpp`     | Provides backend implementation to the BMv2's PNA NIC compiler. |
| `main.cpp`                   | Sets up compilation environment, integrates various components, and executes the PNA NIC compiler. |
| `version.h.cmake`            | Defines macros containing version information for the PNA NIC compiler. |
