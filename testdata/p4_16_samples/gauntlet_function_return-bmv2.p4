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

bit<8> do_thing(out bit<32> d) {
    return (bit<8>)32w1;
}
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    action action_thing(inout bit<32> c) {
        c = (bit<32>)do_thing(c);
    }
    apply {
        action_thing(sm.enq_timestamp);
    }
}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
state start {transition accept;}}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

