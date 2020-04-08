
/*
Copyright 2018 Cisco Systems, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <core.p4>
#include <v1model.p4>

typedef bit<48>  EthernetAddress;

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h    ethernet;
}

struct metadata_t {
    bit<4>  a;
    bit<4>  b;
}

control my_control_type(inout bit<16> x);

control C1(inout bit<16> x)
{
    counter((bit<32>) 65536, CounterType.packets) stats;
    apply {
        x = x + 1;
        stats.count((bit<32>) x);
    }
};

control C2(inout bit<16> x)(my_control_type c)
{
    apply {
        x = x << 1;
        c.apply(x);
    }
}

control C3(inout bit<16> x)(my_control_type c) {
    apply {
        x = x << 3;
        c.apply(x);
    }
}

control E(inout bit<16> x) {
    C1() c1;
    C2(c1) c2;
    C3(c1) c3;
    apply {
        c2.apply(x);
        c3.apply(x);
    }
}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout metadata_t meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr,
                 inout metadata_t meta,
                 inout standard_metadata_t stdmeta) {
    apply {
        E.apply(hdr.ethernet.etherType);
    }
}

control cEgress(inout Parsed_packet hdr,
                inout metadata_t meta,
                inout standard_metadata_t stdmeta) {
    apply { }
}

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply { packet.emit(hdr.ethernet); }
}

control vc(inout Parsed_packet hdr,
           inout metadata_t meta) {
    apply { }
}

control uc(inout Parsed_packet hdr,
           inout metadata_t meta) {
    apply { }
}

V1Switch(parserI(),
    vc(),
    cIngress(),
    cEgress(),
    uc(),
    DeparserI()) main;
