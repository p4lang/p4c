#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header h_index1 {
    bit<8> index;
}

header h_index2 {
    bit<8> index;
}

header h_index {
    bit<8> index;
}

struct headers {
    h_index1 h0_i1;
    h_index2 h0_i2;
    h_index1 h1_i1;
    h_index2 h1_i2;
    h_index  i;
}

struct Meta {
}

parser p(packet_in pkt, out headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<h_index1>(hdr.h0_i1);
        pkt.extract<h_index>(hdr.i);
        hdr.i.index = hdr.h0_i1.index;
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
        pkt.emit<h_index1>(h.h0_i1);
        pkt.emit<h_index>(h.i);
    }
}

V1Switch<headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
