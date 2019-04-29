#include <core.p4>
#include <v1model.p4>

struct counter_metadata_t {
    bit<16> counter_index_first;
    bit<16> counter_index_second;
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
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

@name(".count1") @min_width(32) counter(32w16384, CounterType.packets) count1;

@name(".count2") @min_width(32) counter(32w16384, CounterType.packets) count2;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".set_index") action set_index(bit<16> index1, bit<16> index2, bit<9> port) {
        meta.counter_metadata.counter_index_first = index1;
        meta.counter_metadata.counter_index_second = index2;
        standard_metadata.egress_spec = port;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<32>)meta.counter_metadata.counter_index_first);
    }
    @name(".count_entries2") action count_entries2() {
        count2.count((bit<32>)meta.counter_metadata.counter_index_second);
    }
    @name(".index_setter") table index_setter_0 {
        actions = {
            set_index();
            @defaultonly NoAction_0();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
            hdr.data.f2: exact @name("data.f2") ;
        }
        size = 2048;
        default_action = NoAction_0();
    }
    @name(".stats") table stats_0 {
        actions = {
            count_entries();
        }
        default_action = count_entries();
    }
    @name(".stats2") table stats2_0 {
        actions = {
            count_entries2();
        }
        default_action = count_entries2();
    }
    apply {
        index_setter_0.apply();
        stats_0.apply();
        stats2_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

