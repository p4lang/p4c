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

struct value_set_t {
    @match(ternary)
    bit<16> field1;
    @match(lpm)
    bit<3>  field2;
    @match(exact)
    bit<6>  field3;
    @match(range)
    bit<5>  field4;
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    @name("MyParser.pvs") value_set<value_set_t>(4) pvs_0;
    state start {
        b.extract<data_h>(p.data);
        transition select(p.data.da[15:0], p.data.db[7:5], p.data.db[29:24], p.data.da[30:26]) {
            pvs_0: accept;
            (16w0x810, 3w0x4 &&& 3w0x6, 6w0x32 &&& 6w0x33, 5w10 &&& 5w30): foo;
            (16w0x810, 3w0x4 &&& 3w0x6, 6w0x32 &&& 6w0x33, 5w12 &&& 5w28): foo;
            (16w0x810, 3w0x4 &&& 3w0x6, 6w0x32 &&& 6w0x33, 5w16 &&& 5w28): foo;
            (16w0x810, 3w0x4 &&& 3w0x6, 6w0x32 &&& 6w0x33, 5w20): foo;
            default: noMatch;
        }
    }
    state foo {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
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
