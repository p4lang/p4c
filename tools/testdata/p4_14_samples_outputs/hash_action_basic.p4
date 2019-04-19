#include <core.p4>
#include <v1model.p4>

struct counter_metadata_t {
    bit<16> counter_index;
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<16> h3;
    bit<16> h4;
}

struct metadata {
    @name(".counter_metadata") 
    counter_metadata_t counter_metadata;
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".count1") @min_width(32) counter(32w16384, CounterType.packets) count1;
    @name(".set_index") action set_index(bit<16> index, bit<9> port) {
        meta.counter_metadata.counter_index = index;
        standard_metadata.egress_spec = port;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<32>)(bit<32>)meta.counter_metadata.counter_index);
    }
    @name(".index_setter") table index_setter {
        actions = {
            set_index;
        }
        key = {
            hdr.data.f1: exact;
            hdr.data.f2: exact;
        }
        size = 2048;
    }
    @name(".stats") table stats {
        actions = {
            count_entries;
        }
        default_action = count_entries();
    }
    apply {
        index_setter.apply();
        stats.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
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

