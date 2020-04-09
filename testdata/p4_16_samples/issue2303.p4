#include <core.p4>
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
}


struct Headers {
    ethernet_t eth_hdr;
    H h;
}

struct Meta {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
}


parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control MyC2(inout H h, in Meta meta={ 0, 0, 0 } ) {
    apply {
        h.a = meta.f0;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    MyC2() c2;
    apply {
        c2.apply(h.h);
    }
}




control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out b, in Headers h) { apply {b.emit(h);} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
