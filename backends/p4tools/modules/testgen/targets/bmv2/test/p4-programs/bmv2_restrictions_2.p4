#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> a1;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta { }

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
    bit<8> switch_var = 0;
    action MyAction1(@refers_to(table_1 , h.h.a) bit<8> input_val) {
        h.h.b = input_val;
        switch_var = 1;
    }

    table table_1 {
        key = {
            h.h.a : exact @refers_to(table_2 , h.h.b);
        }

        actions = {
            NoAction;
            MyAction1;
        }

        size = 1024;
        default_action = NoAction();
    }

    table table_2 {
        key = {
            h.h.b : exact;
        }

        actions = {
            NoAction;
            MyAction1;
        }

        size = 1024;
        default_action = NoAction();
    }

    apply {
        table_1.apply();
        // This should not be possible with valid refers_to annotations.
        if (h.h.isValid()) {
            if (switch_var == 1 && h.h.b != h.h.a) {
                h.h.b = 3;
            }
        }
        if (h.h.a == 2) {
            table_2.apply();
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
