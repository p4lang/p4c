#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

enum bit<8> MyEnum1B {
    MBR1 = 8w0,
    MBR2 = 8w0xff
}

enum bit<16> MyEnum2B {
    MBR1 = 16w10,
    MBR2 = 16w0xab00
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
        b.extract<hdr>(h.h);
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
        b.emit<hdr>(h.h);
    }
}

control ingress(inout Header_t h, inout Meta_t m, inout standard_metadata_t standard_meta) {
    @name("ingress.a") action a() {
        standard_meta.egress_spec = 9w0;
    }
    @name("ingress.a_with_control_params") action a_with_control_params(@name("x") bit<9> x) {
        standard_meta.egress_spec = x;
    }
    @name("ingress.t_ternary") table t_ternary_0 {
        key = {
            h.h.f1: exact @name("h.h.f1");
            h.h.f2: ternary @name("h.h.f2");
        }
        actions = {
            a();
            a_with_control_params();
        }
        const default_action = a();
        const entries = {
                        (MyEnum1B.MBR1, default) : a_with_control_params(9w1);
                        (MyEnum1B.MBR2, MyEnum2B.MBR2 &&& 16w0xff00) : a_with_control_params(9w2);
                        (MyEnum1B.MBR2, default) : a_with_control_params(9w3);
        }
    }
    apply {
        t_ternary_0.apply();
    }
}

V1Switch<Header_t, Meta_t>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
