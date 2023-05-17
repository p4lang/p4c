#include <core.p4>
#include <v1model.p4>
bit<3> bound(in bit<3> val, in bit<3> bound_val) {
    return (val > bound_val ? bound_val : val);
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header I {
    bit<3>  a;
    bit<5> padding;
}

header H {
    bit<32>  a;
}

struct Headers {
    ethernet_t eth_hdr;
    I     i;
    H[2]  h;
}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.i);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.h[bound(h.i.a, 3w1)].a = 32w1;
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.i);
        pkt.emit(h.h[0]);
        pkt.emit(h.h[1]);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

