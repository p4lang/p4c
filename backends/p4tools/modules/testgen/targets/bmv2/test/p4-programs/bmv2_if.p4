#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
    bit<8> c;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
    apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {

    action MyAction1() {
        h.h.b = 1;
    }

    action MyAction2() {
        h.h.b = 2;
    }

    action MyAction3() {
        h.h.b = 3;
    }

    action MyAction4() {
        h.h.b = 4;
    }

    action MyAction5() {
        h.h.b = 5;
    }

    action MyAction6() {
        h.h.b = 6;
    }

    table table1 {
        key = {
            h.h.a : exact;
        }

        actions = {
            NoAction;
            MyAction3;
            MyAction4;
        }

        const entries = {
            8w3 : MyAction3();
            8w4 : MyAction4();
        }

        size = 1024;
        default_action = NoAction();
    }

    table table2 {
        key = {
            h.h.a : exact;
        }

        actions = {
            NoAction;
            MyAction5;
            MyAction6;
        }

        const entries = {
            8w5 : MyAction5();
            8w6 : MyAction6();
        }

        size = 1024;
        default_action = NoAction();
    }

    apply {
        if (h.h.b == 0) {
            MyAction1();
        } else {
            MyAction2();
        }
        if (h.h.c > 0) {
            table1.apply();
        } else {
            table2.apply();
        }
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
