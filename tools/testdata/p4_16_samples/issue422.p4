/*
Copyright 2017 VMware, Inc.

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

struct mystruct1 {
    bit<4>  a;
    bit<4>  b;
}

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply { packet.emit(hdr.ethernet); }
}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout mystruct1 meta,
               inout standard_metadata_t stdmeta) {
    // This is acceptable to p4test and p4c-bm2-ss
    const bit<16> c0 = 0xdead;
    bit<16> v0;
    state start {
        // The const line below causes p4test and p4c-bm2-ss to give
        // an error.  It doesn't seem to be an onerous restriction to
        // disallow const declarations inside of states, but variable
        // declarations are allowed here and in top level of parser,
        // so why not const?
        const bit<16> c1 = 0xdead;
        bit<16> v1;
        {
            pkt.extract(hdr.ethernet);
        }
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr,
                 inout mystruct1 meta,
                 inout standard_metadata_t stdmeta) {
    action foo () {
    }
    table tbl1 {
        key = { }
        actions = { foo; NoAction; }
        default_action = NoAction;
    }
    apply {
        tbl1.apply();
    }
}

control cEgress(inout Parsed_packet hdr,
                inout mystruct1 meta,
                inout standard_metadata_t stdmeta) {
    apply { }
}

control vc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

control uc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

V1Switch<Parsed_packet, mystruct1>(parserI(),
                                   vc(),
                                   cIngress(),
                                   cEgress(),
                                   uc(),
                                   DeparserI()) main;
