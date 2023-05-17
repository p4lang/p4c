#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header h_index {
    bit<32> index;
}

header h_stack {
    bit<32> a;
}

struct headers {
    ethernet_t eth_hdr;
    h_stack[3] h;
    h_index    i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.eth_hdr);
        pkt.extract(hdr.h[0]);
        pkt.extract(hdr.h[1]);
        pkt.extract(hdr.h[2]);
        pkt.extract(hdr.i);
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        if (h.h[h.i.index].isValid()) {
            h.h[h.h[h.i.index].a].a = 1;
        }
        h.h[h.i.index].setInvalid();
    }
}

control vrfy(inout headers h, inout Meta m) {
    apply {
    }
}

control update(inout headers h, inout Meta m) {
    apply {
    }
}

control egress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in headers h) {
    apply {
        pkt.emit(h.eth_hdr);
        pkt.emit(h.h[0]);
        pkt.emit(h.h[1]);
        pkt.emit(h.h[2]);
        pkt.emit(h.i);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

