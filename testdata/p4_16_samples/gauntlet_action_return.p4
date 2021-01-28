#include <core.p4>
#include <v1model.p4>

header H {
    bit<8>  a;
}

struct Headers {
    H h;
}

struct Meta {
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    bit<128> tmp_key = 128w2;
    action do_action(inout bit<8> val) {
        if (val > 8w10) {
            val = 8w2;
            return;
        } else{
            val = 8w3;
        }

        return;
        val = 8w4;
    }
    table simple_table {
        key = {
            tmp_key             : exact @name("bKiScA") ;
        }
        actions = {
            do_action(h.h.a);
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

