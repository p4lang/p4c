# PINS P4 Programs

These P4 programs are P4 models of production switches. They are sourced from [sonic-pins repository](https://github.com/sonic-net/sonic-pins/tree/main/sai_p4/instantiations/google). For more information, please see [SONiC website](https://sonic-net.github.io/SONiC/).


To refresh the DASH programs used here, please use the following commands:

```
git clone https://github.com/sonic-net/sonic-pins/
p4test pins-infra/sai_p4/instantiations/google/fabric.p4 --pp testdata/p4_16_samples/pins/pins_fabric.p4
p4test pins-infra/sai_p4/instantiations/google/middleblock.p4 --pp testdata/p4_16_samples/pins/pins_middleblock.p4
p4test pins-infra/sai_p4/instantiations/google/wbb.p4 --pp testdata/p4_16_samples/pins/pins_wbb.p4
```
