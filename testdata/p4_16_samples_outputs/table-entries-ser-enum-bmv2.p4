#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

enum bit<8> MyEnum1B {
    MBR1 = 0,
    MBR2 = 0xff
}

enum bit<16> MyEnum2B {
    MBR1 = 10,
    MBR2 = 0xab00
}

header hdr {
    MyEnum1B f1;
    MyEnum2B f2;
}

struct Header_t {
    hdr h;
}

struct Meta_t {
}

parser p(packet_in b, out Header_t h, inout Meta_t m, inout standard_metadata_t sm) {
    state start {
        b.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Header_t h, inout Meta_t m) {
    apply {
    }
}

control update(inout Header_t h, inout Meta_t m) {
    apply {
    }
}

control egress(inout Header_t h, inout Meta_t m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out b, in Header_t h) {
    apply {
        b.emit(h.h);
    }
}

control ingress(inout Header_t h, inout Meta_t m, inout standard_metadata_t standard_meta) {
    action a() {
        standard_meta.egress_spec = 0;
    }
    action a_with_control_params(bit<9> x) {
        standard_meta.egress_spec = x;
    }
    table t_ternary {
        key = {
            h.h.f1: exact;
            h.h.f2: ternary;
        }
        actions = {
            a;
            a_with_control_params;
        }
        const default_action = a();
        const entries = {
                        (MyEnum1B.MBR1, default) : a_with_control_params(1);
                        (MyEnum1B.MBR2, MyEnum2B.MBR2 &&& 0xff00) : a_with_control_params(2);
                        (MyEnum1B.MBR2, default) : a_with_control_params(3);
        }
    }
    apply {
        t_ternary.apply();
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
