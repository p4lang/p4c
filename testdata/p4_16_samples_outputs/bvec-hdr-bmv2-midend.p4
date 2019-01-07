#include <core.p4>
#include <v1model.p4>

struct row_t {
    bit<8>  e;
    bit<16> t;
    bit<8>  l;
    bit<8>  r;
    bit<8>  v;
}

header hdr {
    bit<8>  _row_e0;
    bit<16> _row_t1;
    bit<8>  _row_l2;
    bit<8>  _row_r3;
    bit<8>  _row_v4;
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
    @name("ingress.a_with_control_params") action a_with_control_params(bit<9> x) {
        standard_meta.egress_spec = x;
    }
    @name("ingress.t_exact") table t_exact_0 {
        key = {
            h.h._row_e0: exact @name("h.h.row.e") ;
        }
        actions = {
            a();
            a_with_control_params();
        }
        default_action = a();
        const entries = {
                        8w0x1 : a_with_control_params(9w1);

                        8w0x2 : a_with_control_params(9w2);

        }

    }
    apply {
        t_exact_0.apply();
    }
}

V1Switch<Header_t, Meta_t>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

