#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_h {
    bit<32> da;
    bit<32> db;
}

struct my_packet {
    data_h data;
}

struct my_metadata {
    data_h[2] data;
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    @name("MyParser.pvs") value_set<bit<32>>(4) pvs_0;
    state start {
        b.extract<data_h>(p.data);
        transition select(p.data.da) {
            pvs_0: accept;
            32w0x810: foo;
        }
    }
    state foo {
        transition accept;
    }
}

control MyVerifyChecksum(inout my_packet hdr, inout my_metadata meta) {
    apply {
    }
}

control MyIngress(inout my_packet p, inout my_metadata meta, inout standard_metadata_t s) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIngress.set_data") action set_data() {
    }
    @name("MyIngress.t") table t_0 {
        actions = {
            set_data();
            @defaultonly NoAction_1();
        }
        key = {
            meta.data[0].da: exact @name("meta.data[0].da");
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

control MyEgress(inout my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    apply {
    }
}

control MyComputeChecksum(inout my_packet p, inout my_metadata m) {
    apply {
    }
}

control MyDeparser(packet_out b, in my_packet p) {
    apply {
    }
}

V1Switch<my_packet, my_metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
