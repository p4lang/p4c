#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

typedef bit<16> l4_port_t;

header H {
    bit<8> a;
    bit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
    bit<32> hash1;
    l4_port_t d_port;
    l4_port_t s_port;
}

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

    action MyAction1(bit<8> val) {
        h.h.b = val;
    }

    table selector_table {
        key = {
            h.h.a : lpm @name("dst_prefix");
            h.eth_hdr.src_addr : selector;
            h.eth_hdr.dst_addr : selector;
            m.d_port : selector;
            m.s_port : selector;
        }

        actions = {
            MyAction1;
        }
        @name("hashed_selector") implementation = action_selector(HashAlgorithm.crc16, 32w1024, 32w16);
        size = 1024;
    }

    apply {
        selector_table.apply();
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
