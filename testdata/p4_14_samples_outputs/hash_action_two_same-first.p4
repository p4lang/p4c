#include <core.p4>
#include <v1model.p4>

struct counter_metadata_t {
    bit<16> counter_index;
}

struct meter_metadata_t {
    bit<16> meter_index;
}

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<16> h2;
    bit<16> h3;
    bit<16> h4;
    bit<16> h5;
    bit<16> h6;
    bit<8>  color_1;
}

struct metadata {
    @name(".counter_metadata") 
    counter_metadata_t counter_metadata;
    @name(".meter_metadata") 
    meter_metadata_t   meter_metadata;
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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".count1") @min_width(32) counter(32w16384, CounterType.packets) count1;
    @name(".meter1") meter(32w1024, MeterType.bytes) meter1;
    @name(".set_index") action set_index(bit<16> index, bit<9> port) {
        meta.counter_metadata.counter_index = index;
        meta.meter_metadata.meter_index = index;
        standard_metadata.egress_spec = port;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<32>)meta.counter_metadata.counter_index);
        meter1.execute_meter<bit<8>>((bit<32>)meta.meter_metadata.meter_index, hdr.data.color_1);
    }
    @name(".index_setter") table index_setter {
        actions = {
            set_index();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: exact @name("data.f1") ;
            hdr.data.f2: exact @name("data.f2") ;
        }
        size = 2048;
        default_action = NoAction();
    }
    @name(".stats") table stats {
        actions = {
            count_entries();
            @defaultonly NoAction();
        }
        default_action = NoAction();
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

