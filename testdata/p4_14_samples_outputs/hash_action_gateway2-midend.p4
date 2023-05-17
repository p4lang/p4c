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
    bit<16> _counter_metadata_counter_index0;
    bit<4>  _counter_metadata_counter_run1;
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
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name(".set_index") action set_index(@name("index") bit<16> index_1, @name("port") bit<9> port) {
        meta._counter_metadata_counter_index0 = index_1;
        standard_metadata.egress_spec = port;
        meta._counter_metadata_counter_run1 = 4w1;
    }
    @name(".count_entries") action count_entries() {
        count1.count((bit<14>)meta._counter_metadata_counter_index0);
    }
    @name(".seth2") action seth2(@name("val") bit<16> val) {
        hdr.data.h2 = val;
    }
    @name(".seth4") action seth4(@name("val") bit<16> val_2) {
        hdr.data.h4 = val_2;
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
    @name(".test1") table test1_0 {
        actions = {
            seth2();
            @defaultonly NoAction_2();
        }
        key = {
            hdr.data.h1: exact @name("data.h1");
        }
        default_action = NoAction_2();
    }
    @name(".test2") table test2_0 {
        actions = {
            seth4();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.data.h2: exact @name("data.h2");
        }
        default_action = NoAction_3();
    }
    apply {
        index_setter_0.apply();
        if (meta._counter_metadata_counter_run1 == 4w1) {
            stats_0.apply();
            test1_0.apply();
        } else {
            test2_0.apply();
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
