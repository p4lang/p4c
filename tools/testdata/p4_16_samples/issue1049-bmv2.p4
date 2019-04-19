/*
Copyright 2017 Cisco Systems, Inc.

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
typedef bit<32>  IPv4Address;

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

// IPv4 header _with_ options
header ipv4_t {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

struct headers {
    ethernet_t    ethernet;
    ipv4_t        ipv4;
}

struct mystruct1_t {
    bit<16>  hash1;
    bool     hash_drop;
}

struct metadata {
    mystruct1_t mystruct1;
}

parser parserI(packet_in pkt,
               out headers hdr,
               inout metadata meta,
               inout standard_metadata_t stdmeta)
{
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

control cIngress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t stdmeta)
{
    action hash_drop_decision() {
        hash(meta.mystruct1.hash1, HashAlgorithm.crc16, (bit<16>) 0,
            { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.protocol },
            (bit<32>) 0xffff);
        meta.mystruct1.hash_drop = (meta.mystruct1.hash1 < 0x8000);
    }
    table guh {
        key = {
            hdr.ipv4.dstAddr : exact;
        }
        actions = {
            hash_drop_decision;
        }
        default_action = hash_drop_decision;
    }
    table debug_table {
        key = {
            meta.mystruct1.hash1     : exact;
            meta.mystruct1.hash_drop : exact;
        }
        actions = { NoAction; }
    }
    apply {
        if (hdr.ipv4.isValid()) {
            meta.mystruct1.hash_drop = false;
            guh.apply();
            debug_table.apply();
            if (meta.mystruct1.hash_drop) {
                hdr.ethernet.dstAddr = meta.mystruct1.hash1 ++ 7w0 ++ (bit<1>) meta.mystruct1.hash_drop ++ 8w0 ++ 16w0xdead;
            } else {
                hdr.ethernet.dstAddr = meta.mystruct1.hash1 ++ 7w0 ++ (bit<1>) meta.mystruct1.hash_drop ++ 8w0 ++ 16w0xc001;
            }
        }
    }
}

control cEgress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t stdmeta)
{
    apply {
    }
}

control verifyChecksum(inout headers hdr,
                       inout metadata meta)
{
    apply {
    }
}

control updateChecksum(inout headers hdr,
                       inout metadata meta)
{
    apply {
    }
}

control DeparserI(packet_out packet,
                  in headers hdr)
{
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

V1Switch<headers, metadata>(parserI(),
                            verifyChecksum(),
                            cIngress(),
                            cEgress(),
                            updateChecksum(),
                            DeparserI()) main;
