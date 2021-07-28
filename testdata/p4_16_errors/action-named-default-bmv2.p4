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

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header H {
    bit<8> a;
    bit<8> b;
}

struct headers_t {
    ethernet_t eth;
    H h;
}

struct metadata_t {
}

control deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr);
    }
}

parser p(packet_in pkt, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.eth);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    action a1() {
        stdmeta.egress_spec = 1;
    }
    action default() {
        stdmeta.egress_spec = 2;
    }
    table simple_table {
        key = {
            hdr.h.b: exact;
        }
        actions = {
            a1();
            default();
        }
        default_action = a1;
    }
    apply {
        bit<8> tmp_condition = 0;
        stdmeta.egress_spec = 0;
        switch (simple_table.apply().action_run) {
            a1: {
                tmp_condition = 1;
            }
            default: {
                tmp_condition = 2;
            }
        }

        if (tmp_condition == 2) {
            hdr.h.a = 0;
        }
    }
}

control egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {}
}

control vrfy(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

control update(inout headers_t hdr, inout metadata_t meta) {
    apply {}
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
