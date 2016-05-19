# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#include "/home/mbudiu/git/p4c/build/../p4include/core.p4"
#include "/home/mbudiu/git/p4c/build/../p4include/v1model.p4"

@name("queueing_metadata_t") struct queueing_metadata_t_0 {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

header hdr1_t {
    bit<8> f1;
    bit<8> f2;
}

header queueing_metadata_t {
    bit<48> enq_timestamp;
    bit<24> enq_qdepth;
    bit<32> deq_timedelta;
    bit<24> deq_qdepth;
}

struct metadata {
    @name("queueing_metadata") 
    queueing_metadata_t_0 queueing_metadata;
}

struct headers {
    @name("hdr1") 
    hdr1_t              hdr1;
    @name("queueing_hdr") 
    queueing_metadata_t queueing_hdr;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("queueing_dummy") state queueing_dummy {
        packet.extract(hdr.queueing_hdr);
        transition accept;
    }
    @name("start") state start {
        packet.extract(hdr.hdr1);
        transition select(standard_metadata.packet_length) {
            32w0: queueing_dummy;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("copy_queueing_data") action copy_queueing_data() {
        hdr.queueing_hdr.setValid();
        hdr.queueing_hdr.enq_timestamp = meta.queueing_metadata.enq_timestamp;
        hdr.queueing_hdr.enq_qdepth = meta.queueing_metadata.enq_qdepth;
        hdr.queueing_hdr.deq_timedelta = meta.queueing_metadata.deq_timedelta;
        hdr.queueing_hdr.deq_qdepth = meta.queueing_metadata.deq_qdepth;
    }
    @name("t_egress") table t_egress() {
        actions = {
            copy_queueing_data;
            NoAction;
        }
        default_action = NoAction();
    }
    apply {
        t_egress.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("set_port") action set_port(bit<9> port) {
        standard_metadata.egress_spec = port;
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("t_ingress") table t_ingress() {
        actions = {
            set_port;
            _drop;
            NoAction;
        }
        key = {
            hdr.hdr1.f1: exact;
        }
        size = 128;
        default_action = NoAction();
    }
    apply {
        t_ingress.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.hdr1);
        packet.emit(hdr.queueing_hdr);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
