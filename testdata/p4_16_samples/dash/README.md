# DASH P4 Programs
The Disaggregated API for SONiC Hosts (DASH) defines a behavioral model for conformance testing of network devices. The P4 programs contained here model various network architecture scenarios related to the DASH project.

They are sourced from [DASH repository on GitHub](https://github.com/sonic-net/DASH). For more information, please see [DASH High-Level Design document](https://github.com/sonic-net/DASH/blob/main/documentation/general/dash-high-level-design.md).

To refresh the DASH programs used here, please use the following commands:

```
git clone https://github.com/sonic-net/DASH
# V1Model version
p4test DASH/dash-pipeline/bmv2/dash_pipeline.p4  -DTARGET_BMV2_V1MODEL --pp dash-pipeline-v1model-bmv2.p4
# PNA Version
p4test DASH/dash-pipeline/bmv2/dash_pipeline.p4  -DTARGET_DPDK_PNA --pp dash-pipeline-pna-dpdk.p4
```
