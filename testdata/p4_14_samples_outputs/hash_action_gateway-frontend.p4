#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct counter_metadata_t {
    bit<16> counter_index;
    bit<4>  counter_run;
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

@name(".count1") @min_width(32) counter<bit<14>>(32w16384, CounterType.packets) count1;
control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".set_index") action set_index(@name("index") bit<16> index_1, @name("port") bit<9> port) {
        meta.counter_metadata.counter_index = index_1;
        standard_metadata.egress_spec = port;
        meta.counter_metadata.counter_run = 4w1;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<14>)meta.counter_metadata.counter_index);
    }
    @name(".index_setter") table index_setter_0 {
        actions = {
            set_index();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.data.f1: exact @name("data.f1");
            hdr.data.f2: exact @name("data.f2");
        }
        size = 2048;
        default_action = NoAction_1();
    }
    @name(".stats") table stats_0 {
        actions = {
            count_entries();
        }
        default_action = count_entries();
    }
    apply {
        index_setter_0.apply();
        if (meta.counter_metadata.counter_run == 4w1) {
            stats_0.apply();
        }
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
