# DPDK backend

The **p4c-dpdk** backend translates the P4-16 programs to DPDK API to configure
the DPDK software switch (SWX) pipeline. DPDK introduced the SWX pipeline in
the DPDK 20.11 LTS release. For more information, please refer to the release
note at https://doc.dpdk.org/guides/rel_notes/release_20_11.html.

The p4c-dpdk compiler accepts the P4-16 programs written for the psa.p4
architecture model.

The backend for dpdk reuses code from the p4c-bm2 "common" library for handling
PSA architecture. Internally, it translates the PSA program to a representation
that conforms to the DPDK SWX pipeline and generates the 'spec' file to
configure the DPDK pipeline.

##How to use it?
The sample p4 programs locate in the "example" directory.

To generate the 'spec' file:

p4c-dpdk vxlan.p4 -o vxlan.spec

To load the 'spec' file in dpdk:

TBD

##Known issues:
TBD

##Contacts:

Han Wang <han2.wang@intel.com>

Cristian Dumitrescu <cristian.dumitrescu@intel.com>
