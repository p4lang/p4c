# DPDK backend

The **p4c-dpdk** backend translates the P4-16 programs to DPDK API to configure
the DPDK software switch (SWX) pipeline. DPDK introduced the SWX pipeline in
the DPDK 20.11 LTS release. For more information, please refer to the release
note at https://doc.dpdk.org/guides/rel_notes/release_20_11.html.

The p4c-dpdk compiler accepts P4-16 programs written for the Portable
Switch Architecture (PSA) (see the [P4.org specifications page](https://p4.org/specs)
for the PSA specification document).

The backend for dpdk reuses code from the p4c-bm2 "common" library for
handling the PSA architecture. Internally, it translates the PSA
program to a representation that conforms to the DPDK SWX pipeline and
generates the 'spec' file to configure the DPDK pipeline.


## How to use it?

A sample P4 program can be found in the `examples` directory.  To
generate the 'spec' file:
```bash
p4c-dpdk vxlan.p4 -o vxlan.spec
```

To load the 'spec' file in dpdk follow the instructions in the
[Pipeline Application User Guide](https://doc.dpdk.org/guides/sample_app_ug/pipeline.html).


## Known issues

TBD


## Contacts

Han Wang <han2.wang@intel.com>

Cristian Dumitrescu <cristian.dumitrescu@intel.com>
