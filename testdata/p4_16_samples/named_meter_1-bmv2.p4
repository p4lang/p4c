/*
Copyright 2016 VMware, Inc.

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

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
}

struct meta_t {
    bit<32> meter_tag;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name("intrinsic_metadata")
    intrinsic_metadata_t intrinsic_metadata;
    @name("meta")
    meta_t               meta;
}

struct headers {
    @name("ethernet")
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name("start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name("namedmeter") direct_meter<bit<32>>(MeterType.packets) my_meter;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("_nop") action _nop() {
    }
    @name("m_action") action m_action(bit<9> meter_idx) {
        standard_metadata.egress_spec = meter_idx;
        standard_metadata.egress_spec = 9w1;
    }
    @name("m_filter") table m_filter {
        actions = {
            _drop;
            _nop;
            NoAction;
        }
        key = {
            meta.meta.meter_tag: exact;
        }
        size = 16;
        default_action = NoAction();
    }
    @name("m_action") action m_action_0(bit<9> meter_idx) {
        standard_metadata.egress_spec = meter_idx;
        my_meter.read(meta.meta.meter_tag);
    }
    @name("_nop") action _nop_0() {
        my_meter.read(meta.meta.meter_tag);
    }
    @name("m_table") table m_table {
        actions = {
            m_action_0;
            _nop_0;
            NoAction;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 16384;
        default_action = NoAction();
        meters = my_meter;
    }
    apply {
        m_table.apply();
        m_filter.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
