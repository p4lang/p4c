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

    action MyAction1() {
        h.h.b = 1;
    }

    action MyAction2() {
        h.h.b = 2;
    }

    action MyAction3() {
        h.h.b = 3;
    }
    @entry_restriction("
    h.h.a1::mask != 0 && h.h.a1::prefix_length != 64; 
    h.h.a1 != 0;
    h.h.a != 0xFF;
    ")
    table ternary_table {
        key = {
            h.h.isValid() : exact @name("valid");
            h.h.a : exact;
            h.h.a1 : ternary;
        }

        actions = {
            NoAction;
            MyAction1;
            MyAction2;
            MyAction3;
        }

        size = 1024;
        default_action = NoAction();
    }

    apply {
      ternary_table.apply();
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
