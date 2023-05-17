/*
Copyright 2019 Orange

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
#define UBPF_MODEL_VERSION 20200515
#include <ubpf_model.p4>

@ethernetaddress typedef bit<48> EthernetAddress;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

struct Headers_t
{
    Ethernet_h ethernet;
}

struct metadata {
    bit<16> etherType;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {

    action fill_metadata() {
        meta.etherType = headers.ethernet.etherType;
    }

    table tbl {
        key = {
            headers.ethernet.etherType : exact;
        }
        actions = {
            fill_metadata;
            NoAction;
        }
    }

    action change_etherType() {
        // set etherType to IPv6. Just to show that metadata works.
        headers.ethernet.etherType = 0x86DD;
    }

    table meta_based_tbl {
        key = {
            meta.etherType : exact;
        }
        actions = {
            change_etherType;
            NoAction;
        }
    }

    apply {
        tbl.apply();
        meta_based_tbl.apply();
    }

}

control DeparserImpl(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
    }
}

ubpf(prs(), pipe(), DeparserImpl()) main;

