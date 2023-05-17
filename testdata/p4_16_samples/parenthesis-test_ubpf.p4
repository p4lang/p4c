/*
Copyright 2020 MNK Labs & Consulting

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

#include <ubpf_model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32>     IPv4Address;

header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

header IPv4_h {
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

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
    bit<8> qfi;
    bit<8> filt_dir;
    bit<8> reflec_qos;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ipv4;
            default : reject;
        }
    }

    state ipv4 {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action Reject() {
        mark_to_drop();
    }

    table filter_tbl {
        key = {
            headers.ipv4.srcAddr : exact;
        }
        actions = {
            Reject;
            NoAction;
        }

        default_action = NoAction;
    }

    apply {
        if (headers.ipv4.isValid())
            filter_tbl.apply();
        if ((meta.qfi != 0) &&
            ((meta.filt_dir == 2) ||
             (meta.reflec_qos == 1)))
	     meta.qfi = 3;
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}


ubpf(prs(), pipe(), dprs()) main;
