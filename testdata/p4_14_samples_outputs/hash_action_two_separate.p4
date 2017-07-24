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

struct __metadataImpl {
    @name("counter_metadata") 
    counter_metadata_t counter_metadata;
}

struct __headersImpl {
    @name("data") 
    data_t data;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        transition accept;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    @name(".count1") @min_width(32) counter(32w16384, CounterType.packets) count1;
    @name(".count2") @min_width(32) counter(32w16384, CounterType.packets) count2;
    @name(".set_index") action set_index(bit<16> index1, bit<16> index2, bit<9> port) {
        meta.counter_metadata.counter_index_first = index1;
        meta.counter_metadata.counter_index_second = index2;
        standard_metadata.egress_spec = port;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<32>)(bit<32>)meta.counter_metadata.counter_index_first);
    }
    @name(".count_entries2") action count_entries2() {
        count2.count((bit<32>)(bit<32>)meta.counter_metadata.counter_index_second);
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
    @name(".stats2") table stats2 {
        actions = {
            count_entries2;
        }
        default_action = count_entries2();
    }
    apply {
        index_setter.apply();
        stats.apply();
        stats2.apply();
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit(hdr.data);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch(__ParserImpl(), __verifyChecksumImpl(), ingress(), egress(), __computeChecksumImpl(), __DeparserImpl()) main;
