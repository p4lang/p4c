#include <core.p4>

const int A = 5;
const bit<16> B = 16w5;
@metadata @name("standard_metadata") struct standard_metadata_t {
    bit<9>  ingress_port;
    bit<9>  egress_spec;
    bit<9>  egress_port;
    bit<32> clone_spec;
    bit<32> instance_type;
    bit<1>  drop;
    bit<16> recirculate_port;
    bit<16> packet_length;
}

header hdr {
    bit<8> e;
}

control c(in hdr h, inout standard_metadata_t standard_meta) {
    action a() {
        standard_meta.egress_port = 9w0;
        standard_meta.packet_length = standard_meta.packet_length + 16w65531;
    }
    action b() {
        standard_meta.egress_port = 9w1;
        standard_meta.packet_length = standard_meta.packet_length + 16w65531;
    }
    table t {
        key = {
            h.e: exact @name("h.e") ;
        }
        actions = {
            a();
            b();
        }
        default_action = a();
        const entries = {
                        8w0x1 : a();

                        8w0x2 : b();

        }

    }
    apply {
        t.apply();
    }
}

control proto(in hdr h, inout standard_metadata_t s);
package top(proto p);
