#include <core.p4>
#include <v1model.p4>

header H {
    bit<16> a;
}


struct Headers {
    H h;
}

struct Meta {
}

bit<16> do_thing_function() {
    H not_initialized;
    bit<32> new_val = 32w1;
    if (not_initialized.a < 6) {
    } else {
        new_val = 32w232;
    }
    return (bit<16>)new_val;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action do_thing_action() {
        do_thing_function();
    }
    apply {
        h.h.a = do_thing_function();
        do_thing_action();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
