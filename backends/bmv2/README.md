# Behavioral Model Backend

This is a back-end which generates code for the Behavioral Model version 2 (BMv2).
https://github.com/p4lang/behavioral-model.git

It can accept either P4_14 programs, or P4_16 programs written for the
`v1model.p4` switch model.
# Backend source code organization

```
bmv2/
├── common/
│   ├── JsonObjects.cpp         -- Sample Description 1 
│   ├── JsonObjects.h           -- Sample Description 2
│   ├── action.cpp              --
│   ├── action.h                --
│   ├── annotations.h
│   ├── backend.h
│   ├── control.h
│   ├── controlFlowGraph.cpp
│   ├── controlFlowGraph.h
│   ├── deparser.cpp
│   ├── deparser.h
│   ├── expression.cpp
│   ├── expression.h
│   ├── extern.cpp
│   ├── extern.h
│   ├── globals.cpp
│   ├── globals.h
│   ├── header.cpp
│   ├── header.h
│   ├── helpers.cpp
│   ├── helpers.h
│   ├── lower.cpp
│   ├── lower.h
│   ├── metermap.cpp
│   ├── metermap.h
│   ├── midend.h
│   ├── options.h
│   ├── parser.cpp
│   ├── parser.h
│   ├── programStructure.cpp
│   ├── programStructure.h
│   └── sharedActionSelectorCheckh
├── psa_switch/
│   ├── main.cpp
│   ├── midend.cpp
│   ├── midend.h
│   ├── options.cpp
│   ├── options.h
│   ├── psaProgramStructure.cpp
│   ├── psaProgramStructure.h
│   ├── psaSwitch.cpp
│   ├── psaSwitch.h
│   └── version.h.cmake
├── simple_switch/
│   ├── main.cpp
│   ├── midend.cpp
│   ├── midend.h
│   ├── options.h
│   ├── simpleSwitch.cpp
│   ├── simpleSwitch.h
│   └── version.h.cmake
├── .gitignore
├── CMakeLists.txt
├── README.md
├── bmv2.def
├── bmv2stf.py
├── run-bmv2-ptf-test.py
└── run-bmv2-test.py
```

# Dependencies

To run and test this back-end you need some additional tools:

- the BMv2 behavioral model itself.  Installation instructions are available at
  https://github.com/p4lang/behavioral-model.git.  You may need to update your
  dynamic libraries after installing bmv2: `sudo ldconfig`

- the Python scapy library `sudo pip3 install scapy`

# Unsupported P4_16 language features

Here are some unsupported features we are aware of. We will update this list as
more features get supported in the bmv2 compiler backend and as we discover more
issues.

- explicit transition to reject in parse state

- compound action parameters (can only be `bit<>` or `int<>`)

- functions or methods with a compound return type
```
struct s_t {
    bit<8> f0;
    bit<8> f1;
};

extern s_t my_extern_function();

controlc c() {
    apply { s_t s1 = my_extern_function(); }
}
```

- user-defined extern types / methods which are not defined in `v1model.p4`

- stacks of header unions
