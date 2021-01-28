#include <core.p4>
#include <v1model.p4>

header H {
    bit<32> a;
}


struct Headers {
    H h;
}

struct Meta {
}

bit<32> simple_function() {
    H[2] tmp1;
    H[2] tmp2;

    if (tmp2[0].a <= 3) {
        tmp1[0] = tmp2[1];
        tmp2[1] = tmp1[1];
    }
    return tmp1[0].a;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action simple_action() {
        h.h.a = (bit<32>)(simple_function());

    }
    table simple_table {
        key = {
            sm.egress_spec        : exact @name("key") ;
        }
        actions = {
            simple_action();
        }
    }
    apply {
        simple_table.apply();
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

