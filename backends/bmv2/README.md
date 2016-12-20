This is a back-end which generates code for the Behavioral Model version 2 (BMv2).
https://github.com/p4lang/behavioral-model.git

It can accept either P4_14 programs, or P4_16 programs written for the
`v1model.p4` switch model.

# Dependences

To run and test this back-end you need some additional tools:

- the BMv2 behavioral model itself.  Installation instructions are available at
  https://github.com/p4lang/behavioral-model.git.  You may need to update your
  dynamic libraries after installing bmv2: `sudo ldconfig`

- the Python scapy library for manipulating network packets `sudo pip install scapy`

- the Python ipaddr library `sudo pip install ipaddr`
