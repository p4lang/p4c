#include <core.p4>
#include <v1model.p4>

header hdr {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8>  r;
    bit<8>  v;
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
    action a() {
        standard_meta.egress_spec = 9w0;
    }
    action a_with_control_params(bit<9> x) {
        standard_meta.egress_spec = x;
    }
    table t_range {
        key = {
            h.h.r: range @name("h.h.r") ;
        }
        actions = {
            a();
            a_with_control_params();
        }
        default_action = a();
        const entries = {
                        8w1 .. 8w8 : a_with_control_params(9w21);

                        8w6 .. 8w12 : a_with_control_params(9w22);

                        8w15 : a_with_control_params(9w24);

                        default : a_with_control_params(9w23);

        }

    }
    apply {
        t_range.apply();
    }
}

V1Switch<Header_t, Meta_t>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

