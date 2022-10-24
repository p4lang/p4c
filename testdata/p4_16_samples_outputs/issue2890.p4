#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header E {
    bit<16> e;
}

header H {
    bit<16> a;
}

struct Headers {
    E e;
    H h;
}

struct Meta {
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition parse_hdrs;
    }
    state parse_hdrs {
        pkt.extract(hdr.e);
        pkt.extract(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    H x;
    apply {
        x = h.h;
        x.a = 16w2;
        h.e.e = x.a;
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
