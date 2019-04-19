#include <core.p4>
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

struct inner_value_set_t {
    bit<32> field;
}

struct value_set_t {
    bit<32>           field;
    inner_value_set_t inner;
}

parser MyParser(packet_in b, out my_packet p, inout my_metadata m, inout standard_metadata_t s) {
    value_set<value_set_t>(4) pvs;
    state start {
        b.extract(p.data);
        transition select(p.data.da, p.data.da) {
            pvs: accept;
            (0x810, 0x810): foo;
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
    action set_data() {
    }
    table t {
        actions = {
            set_data;
        }
        key = {
            meta.data[0].da: exact;
        }
    }
    apply {
        t.apply();
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

V1Switch(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

