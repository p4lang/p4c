/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

/* 
 * Created by Andy Fingerhut (andy_fingerhut@alum.wustl.edu) as a
 * smaller version of a P4 program written by Aditya Srivastava that
 * exhibited a bug in the latest version of p4c as of 2024-Jan.
 */

#include <core.p4>
#include <v1model.p4>

struct addr_t {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
}

header ethernet_t {
    addr_t  dstAddr;
    addr_t  srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {   
    apply {  }
}

control MyIngress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    bit<8> x;
    apply {
        if (hdr.ethernet.etherType == 0x22f0) {
            x = 0;
            if (hdr.ethernet.srcAddr.f2 == 0xa) {
                hdr.ethernet.srcAddr.f1 = 0xf0;
            } else if (hdr.ethernet.srcAddr.f2 == 0xb) {
                if (hdr.ethernet.srcAddr.f0 == 0xf0) {
                    x = hdr.ethernet.srcAddr.f1;
                } else if (hdr.ethernet.srcAddr.f0 == 0xe0) {
                    x = hdr.ethernet.srcAddr.f1;
                }
            }
            hdr.ethernet.etherType[7:0] = x;
        }
        standard_metadata.egress_spec = 1;
    }
}

control MyEgress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {  }
}

control MyComputeChecksum(inout headers  hdr, inout metadata_t meta) {
     apply {  }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

V1Switch(
    MyParser(),
    MyVerifyChecksum(),
    MyIngress(),
    MyEgress(),
    MyComputeChecksum(),
    MyDeparser()
) main;
