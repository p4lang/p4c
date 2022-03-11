#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_index1 {
    bit<8> index;
}

header h_index2 {
    bit<8> index;
}

header_union h_stack {
    h_index1 i1;
    h_index2 i2;
}

header h_index {
    bit<8> index;
}

struct headers {
    h_stack[2] h;
    h_index    i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.h[0].i1);
        pkt.extract(hdr.i);
        hdr.i.index = hdr.h[0].i1.index;
        transition accept;
    }
}

control ingress(inout headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
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
        pkt.emit(h.h[0].i1);
        pkt.emit(h.i);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
