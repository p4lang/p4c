#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    varbit<8> b;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h1;
    H          h2;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        pkt.extract(h.h1, 10);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h2, 8);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
  apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply {
        if( h.h1.b == h.h2.b) {
            h.eth_hdr.eth_type = 5;
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