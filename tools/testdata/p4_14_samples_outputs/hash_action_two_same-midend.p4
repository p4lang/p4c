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
    bit<16> _counter_metadata_counter_index0;
    bit<16> _meter_metadata_meter_index1;
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
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name(".count1") @min_width(32) counter(32w16384, CounterType.packets) count1_0;
    @name(".meter1") meter(32w1024, MeterType.bytes) meter1_0;
    @name(".set_index") action set_index(bit<16> index, bit<9> port) {
        meta._counter_metadata_counter_index0 = index;
        meta._meter_metadata_meter_index1 = index;
        standard_metadata.egress_spec = port;
    }
    @name(".count_entries") action count_entries() {
        count1_0.count((bit<32>)meta._counter_metadata_counter_index0);
        meter1_0.execute_meter<bit<8>>((bit<32>)meta._meter_metadata_meter_index1, hdr.data.color_1);
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
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        index_setter_0.apply();
        stats_0.apply();
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

