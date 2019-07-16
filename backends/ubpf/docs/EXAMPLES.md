# Introduction

This file contains description of the basic P4 programs, which were used to test the functionality of the P4-to-uBPF compiler.
All tests have been run on the [Oko](https://github.com/Orange-OpenSource/oko) switch using the [Vagrant prepared for Oko](https://github.com/P4-Research/vagrant-oko).

Before any experiment the following commands need to be invoked:

```bash
# compile P4 program to C code
$ p4c-ubpf -o test.c PROGRAM.p4 
$ sudo ovs-ofctl del-flows br0
# compile test.c to BPF machine code
$ clang-3.8 -O2 -I .. -target bpf -v  -c test.c -o /tmp/test.o
# Load filter BPF program and setup rules to forward traffic
$ sudo ovs-ofctl load-filter-prog br0 1 /tmp/test.o
$ sudo ovs-ofctl add-flow br0 table=1,ip,nw_dst=172.16.0.12,actions=output:1
$ sudo ovs-ofctl add-flow br0 priority=1,in_port=2,filter_prog=1,actions=goto_table:1
$ sudo ovs-ofctl add-flow br0 in_port=1,filter_prog=1,actions=output:2
```


## Packet modification

This section presents P4 program, which modifies the packet's fields.

### IPv4 + MPLS (oko-test-actions.p4)

**Key:** Source IPv4 address

**Actions:**

* mpls_decrement_ttl
* mpls_set_label 
* mpls_set_label_decrement_ttl
* mpls_modify_tc
* mpls_set_label_tc
* mpls_modify_stack
* change_ip_ver
* ip_swap_addrs
* ip_modify_saddr 
* Reject

Sample usage:

```bash
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 0 0 0 0 0 0 0 0 0 0 0 0 # decrements MPLS TTL
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 1 0 0 0 24 0 0 0 0 0 0 0 # sets MPLS label to 24
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 2 0 0 0 24 0 0 0 0 0 0 0 # sets MPLS label to 24 and decrements TTL
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 3 0 0 0 3 0 0 0 0 0 0 0 # modifies MPLS TC (set value to 3)
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 4 0 0 0 24 0 0 0 1 0 0 0 # sets MPLS label to 24 and TC to 1
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 5 0 0 0 1 0 0 0 0 0 0 0 # modifies stack value of MPLS header
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 6 0 0 0 6 0 0 0 0 0 0 0 # changes IP version to 6.
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 7 0 0 0 0 0 0 0 0 0 0 0 # swaps IP addresses
$ sudo ovs-ofctl update-map br0 1 0 key 14 0 16 172 value 8 0 0 0 1 0 16 172 0 0 0 0 # sets source IP address to 172.16.0.1
```

### IPv6 (oko-test-ipv6.p4)

The aim of this example is to test modification of wider packet's fields. Thus, we have used the IPv6 headers.

The match key is source IPv6 address. The P4 program implements three actions:

* ipv6_modify_dstAddr - modifies IPv6 destination address;
* ipv6_swap_addr - swaps IPv6 addresses.
* set_flowlabel - tests modification of custom-length field, where `length % 8 != 0`.

Sample usage:
```bash
# Changes destination IPv6 address from fe80::a00:27ff:fe7e:b95 to e00::: (simple, random value)
$ sudo ovs-ofctl update-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 0 0 0 0 14 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
# Swaps source and destination IPv6 addresses
$ sudo ovs-ofctl update-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
# Sets Flow Label to 1.
$ sudo ovs-ofctl update-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 2 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```

## Registers

TBD

## Counters

TBD