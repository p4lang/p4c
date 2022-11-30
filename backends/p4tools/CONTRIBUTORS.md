# Overview 

P4Testgen is a test oracle for the P4 language. Given a P4_16 program and a specification of the underlying architecture, it automatically generates a comprehensive set of input/output tests that can be executed to validate a target device.

# Core Developers

* @fruffy (NYU)
* @liujed (Akita Software)
* @pkotikal (Intel)
* @vhavel (Intel)
* @hannelita (Intel)
* @vladyslav-dubina (Litsoft)
* @VolodymyrPeschanenko (Litsoft)
* @jnfoster (Cornell and Intel)

# History

Jed Liu was the original architect of P4Testgen and designed the abstract machine that allows using P4C's visitors to implement arbitrary traversals of the program IR. Building on Jed's design, Fabian Ruffy completed the symbolic interpreter for P4_16's core features and designed extensions for common open-source architectures. He also developed the model of packets, taint tracking, and concolic execution. Prathima Kotikalapudi developed the extensible test backend, with instantiations for PTF and STF. Vojtěch Havel assisted with the implementation of the symbolic interpreter and the development of P4Testgen extensions for several Intel architectures. Hanneli Tavante added support for strategies that guide the exploration of paths that maximize various notions of coverage. Volodymyr Peschanenko, and Vlad Dubina developed the SMT-LIB interface to Z3, implemented front-end and mid-end passes such as parser unrolling, and completed the back end for the V1Model architecture on the BMv2 simple switch. Nate Foster guided the design of P4Testgen and contributed to various aspects of the symbolic interpreter.
