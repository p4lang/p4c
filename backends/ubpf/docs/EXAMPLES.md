# Introduction

This file contains description of the basic P4 programs, which were used to test the functionality of the P4-to-uBPF compiler.
All tests have been run on the [P4rt-OVS](https://github.com/Orange-OpenSource/p4rt-ovs) switch.
You can use [Vagrantfile](../tests/environment/Vagrantfile) to set up a test environment.

Before any experiment the following commands need to be invoked:

```bash
# compile P4 program to C code
$ p4c-ubpf -o test.c PROGRAM.p4 
$ sudo ovs-ofctl del-flows br0
# compile test.c to BPF machine code
$ clang-6.0 -O2 -I .. -target bpf -c test.c -o /tmp/test.o
# Load filter BPF program
$ sudo ovs-ofctl load-bpf-prog br0 1 /tmp/test.o
# Setup rules to forward traffic (bidirectional)
$ sudo ovs-ofctl add-flow br0 in_port=2,actions=prog:1,output:1
$ sudo ovs-ofctl add-flow br0 in_port=1,actions=prog:1,output:2
```

**Note!** The P4-uBPF compiler works properly with `clang-6.0`. We noticed some problems when using older versions of clang (e.g. 3.9).

# Examples

This section presents how to run and test the P4-uBPF compiler. 

## Packet modification

This section presents a P4 program, which modifies the packet's fields.

### IPv4 + MPLS (simple-actions.p4)

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
# Template: ovs-ofctl update-bpf-map <BRIDGE> <PROGRAM-ID> <MAP-ID> key <KEY-DATA> value <VALUE-DATA>
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 0 0 0 0 0 0 0 0 0 0 0 0 # decrements MPLS TTL
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 1 0 0 0 24 0 0 0 0 0 0 0 # sets MPLS label to 24
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 2 0 0 0 24 0 0 0 0 0 0 0 # sets MPLS label to 24 and decrements TTL
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 3 0 0 0 3 0 0 0 0 0 0 0 # modifies MPLS TC (set value to 3)
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 4 0 0 0 24 0 0 0 1 0 0 0 # sets MPLS label to 24 and TC to 1
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 5 0 0 0 1 0 0 0 0 0 0 0 # modifies stack value of MPLS header
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 6 0 0 0 6 0 0 0 0 0 0 0 # changes IP version to 6.
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 7 0 0 0 0 0 0 0 0 0 0 0 # swaps IP addresses
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 14 0 16 172 value 8 0 0 0 1 0 16 172 0 0 0 0 # sets source IP address to 172.16.0.1
```

### IPv6 (ipv6-actions.p4)

The aim of this example is to test modification of wider packet's fields. Thus, we have used the IPv6 headers.

The match key is source IPv6 address. The P4 program implements three actions:

* ipv6_modify_dstAddr - modifies IPv6 destination address;
* ipv6_swap_addr - swaps IPv6 addresses.
* set_flowlabel - tests modification of custom-length field, where `length % 8 != 0`.

Sample usage:
```bash
# Changes destination IPv6 address from fe80::a00:27ff:fe7e:b95 to e00::: (simple, random value)
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 0 0 0 0 14 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
# Swaps source and destination IPv6 addresses
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
# Sets Flow Label to 1.
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 254 128 0 0 0 0 0 0 10 0 39 255 254 21 180 17 value 2 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```

## Registers

This section presents P4 programs, which use registers. Register can be declared this way:
  
`Register<value_type, key_type>(number_of_elements) register_t;`  

The parameters are as follows:

- `value_type` - is bit array type (i.e. bit<32>) or struct like type  
- `key_type` - is bit array type (i.e. bit<32>) or struct like type  
- `number_of_elements` - the maximum number of key-value pairs

Currently, the `ubpf` architecture model does not allow to initialize registers with default values. 
Initialization has to be done by a control plane. 

### Rate limiter (rate-limiter.p4)

The rate limiter uses two registers. First which counts the number of packets and second which holds timestamps.

This rate limiter limits the number of packets per second. 
Responsible for that are two variables BUCKET_SIZE and WINDOW_SIZE placed in rate-limiter.p4 file. 
For instance now BUCKET_SIZE has value of 10 and WINDOW_SIZE has value of 100. 
It means that 10 packets are passed in 100 ms window. It also means 100 packets per second. 
If you send 1470 Bytes width packets the bit rate should not exceed 1.176 Mbit/s (1470B * 8 * (10/100ms)).

Due to registers limitation before starting your own tests initialize rate limiter registers with zeros:

```bash
# Initalizes timestamp_r register
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 0 0 0 0 value 0 0 0 0
# Initalizes count_r register
$ sudo ovs-ofctl update-bpf-map br0 1 1 key 0 0 0 0 value 0 0 0 0
```

To measure the bandwidth use the `iperf` tool:  
  
Start a `iperf` UDP server

```bash
$ iperf -s -u
```

Then, run `iperf` client:

```bash
$ iperf -c <server_ip> -b 10M -l 1470
```

### Rate limiter (rate-limiter-structs.p4)

The same rate limiter as above, but implemented using structs.  

### Packet counter (packet-counter.p4)

The packet counter counts every packet passed via the program.
Before tests initialize packet counter register with zeros:  

```bash
# Initalizes packet_counter_reg register
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 0 0 0 0 value 0 0 0 0
```

Then generate any network traffic. To check if the program counts packets use i.e.  

```bash
$ watch sudo ovs-ofctl dump-bpf-map br0 1 0
```

### Simple firewall (simple-firewall.p4)

This is very simple example of stateful firewall. Every TCP packet is analyzed to track the state of the TCP connection. 
If the traffic belongs to known connection it is passed. Otherwise, it is dropped.  
Notice that the example program uses hash function which is constrained to hash only 64 bit values - that's why TCP connection is identified via IP source and destination address. 
This is the known limitation of the `uBPF` backend used in P4rt-OVS (to be fixed in the future).
                        
Due to registers limitation before starting your own tests initialize simple firewall registers with zeros:

```bash
# Initalizes conn_state register (key is a output from a hash function for client(192.168.1.10) and server (192.168.1.1))
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 172 192 20 5 value 0 0 0 0
# Initalizes conn_srv_addr register (key is a output from a hash function for client(192.168.1.10) and server (192.168.1.1))
$ sudo ovs-ofctl update-bpf-map br0 1 1 key 172 192 20 5 value 0 0 0 0
```  

To test simple firewall you can use as an example `ptf/simple-firewall-test.py` test.

## Tunneling

This section presents more complex examples of packet tunneling operations. There are two P4 programs used:

* `tunneling.p4`, which implements MPLS tunneling,
* `vxlan.p4`, which implements more complex packet tunneling: VXLAN.

### VXLAN

To run example compile `vxlan.p4` with `p4c` and then `clang-6.0`. 

Sample usage:

```bash
# Sets action vxlan_decap() (value 0) for packets matching rule VNI=25 (key 25) 
# handled by the table upstream_tbl (map id 0) and BPF prog 1.
sudo ovs-ofctl update-bpf-map br0 1 0 key 25 0 0 0 value 0 0 0 0
# Sets action vxlan_encap() (value 0) for packets matching rule IP dstAddr=172.16.0.14 (key 14 0 16 172) 
# handled by the table downstream_tbl (map id 1) and BPF prog 1.
sudo ovs-ofctl update-bpf-map br0 1 1 key 14 0 16 172 value 0 0 0 0
```

### GPRS Tunneling Protocol (GTP)

To run example compile `gtp.p4` with `p4c` and then `clang-6.0`. 

To test encapsulation:

```bash
# For downstream_tbl (ID 1) sets action gtp_encap() (value 0) and GTP TEID=3 for packets with destination IP address 172.16.0.14.
$ sudo ovs-ofctl update-bpf-map br0 1 1 key 14 0 16 172 value 0 0 0 0 3 0 0 0
```

To test decapsulation:

```bash
# For upstream_tbl (ID 0) sets action gtp_decap() for packets matching GTP TEID=3.
$ sudo ovs-ofctl update-bpf-map br0 1 0 key 3 0 0 0 value 0 0 0 0
```

Scapy can be used to easily test GTP protocol:

```python
>>> load_contrib('gtp')
>>> p = Ether(dst='08:00:27:7e:0b:95', src='08:00:27:15:b4:11')/IP(dst='172.16.0.14', src='172.16.0.12')/UDP(sport=2152,dport=2152)/GTPHeader(teid=3)/IP(dst='172.16.0.14', src='172.16.0.12')/ICMP()
>>> sendp(p, iface='eth1')
```