/*
* Copyright 2020, MNK Labs & Consulting
* http://mnkcg.com
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#include <v1model.p4>

typedef bit<48> mac_addr_t;

header aggregator_t {
    bit<8> val;
}
header vec_e_t {
    bit<8> e;
}

header ml_hdr_t {
    int<8> idx;
}

header ethernet_t {
    mac_addr_t dstAddr;
    mac_addr_t srcAddr;
    bit<16>    etherType;
}

struct headers {
    ethernet_t ethernet;
    ml_hdr_t   ml;
    vec_e_t[3] vector;
    aggregator_t[3] pool;
}

struct metadata_t {}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta,
                inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.ethernet);
        packet.extract(hdr.ml);
        packet.extract(hdr.vector[0]);
        packet.extract(hdr.vector[1]);
        packet.extract(hdr.vector[2]);
        packet.extract(hdr.pool[0]);
        packet.extract(hdr.pool[1]);
        packet.extract(hdr.pool[2]);
	transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t meta,
                inout standard_metadata_t standard_metadata) {
    apply {
        hdr.vector[0].e = hdr.pool[1].val;
        hdr.pool[hdr.ml.idx].val = hdr.vector[0].e;
        hdr.pool[hdr.ml.idx].val = hdr.pool[hdr.ml.idx].val + 1;
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
}

control egress(inout headers hdr, inout metadata_t meta,
               inout standard_metadata_t standard_metadata) {
    apply {}
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {}
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {}
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

V1Switch(MyParser(), MyVerifyChecksum(), ingress(), egress(),
MyComputeChecksum(), MyDeparser()) main;
