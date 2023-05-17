#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<32> a;
    bit<32> b;
    bit<8> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H h;
}

struct Meta {
    bit<8> test;
}

control compute(inout H h) {
    @name("a") action a_0() {
        h.b = h.a;
    }
    @name("t") table t_0 {
        key = {
            h.a + h.a: exact @name("e") ;
        }
        actions = {
            a_0();
            NoAction();
        }
        default_action = NoAction();
    }
    apply {
        bit<8> tmp = 0;
        Meta m = {0};
        t_0.apply();
        tmp = 0;
    }
}


parser p(packet_in pkt, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract(h.eth_hdr);
        pkt.extract<H>(h.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("c") compute() c_0;
    apply {
        m.test = 1;
        bit<8> tmp = 1;
        c_0.apply(h.h);
        h.h.c = tmp;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

