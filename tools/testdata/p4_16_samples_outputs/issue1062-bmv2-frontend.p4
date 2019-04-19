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
    error my_err;
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
    @name("ingress.set_error_idx") action set_error_idx(bit<8> idx) {
        h.h.e = idx;
    }
    @name("ingress.t_exact") table t_exact_0 {
        key = {
            m.my_err: exact @name("m.my_err") ;
        }
        actions = {
            set_error_idx();
        }
        default_action = set_error_idx(8w0);
        const entries = {
                        error.NoError : set_error_idx(8w1);

        }

    }
    apply {
        t_exact_0.apply();
    }
}

V1Switch<Header_t, Meta_t>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

