#include <core.p4>
#include <v1model.p4>
bit<1> max(in bit<1> val) {
    return val;
}
header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8>  a;
}

header I {
    bit<1> id;
    bit<7> padding;
}

struct Headers {
    ethernet_t eth_hdr;
    H[2]  h;
    I i;

}

struct Meta {}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.h.next);
        pkt.extract(hdr.i);
        transition accept;
    }
}

bit<8> assign_id(inout bit<1> id) {
    id = 0;
    return 1;
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {

    apply {
        h.h[max(h.i.id)].a = assign_id(h.i.id);
    }
}

control vrfy(inout Headers h, inout Meta m) { apply {} }

control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.h[0]);
        pkt.emit(h.h[1]);
        pkt.emit(h.i);
    }
}
V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

