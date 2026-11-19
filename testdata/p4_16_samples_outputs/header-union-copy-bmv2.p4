#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> value;
}

header_union U {
    H first;
    H second;
}

struct Headers {
    H tag;
    U src;
    U dst;
}

struct Metadata {
}

parser Parse(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    U parsed;
    state start {
        pkt.extract(hdr.tag);
        transition select(hdr.tag.value) {
            0: first;
            1: second;
            default: copy;
        }
    }
    state first {
        pkt.extract(parsed.first);
        transition copy;
    }
    state second {
        pkt.extract(parsed.second);
        transition copy;
    }
    state copy {
        hdr.src = parsed;
        transition accept;
    }
}

control Verify(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control Update(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control TestEgress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control TestIngress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    apply {
        hdr.dst.first.setValid();
        hdr.dst.first.value = 0xee;
        hdr.dst = hdr.src;
        hdr.dst = hdr.dst;
        sm.egress_spec = 1;
    }
}

control Deparse(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit(hdr.tag);
        pkt.emit(hdr.dst);
    }
}

V1Switch(Parse(), Verify(), TestIngress(), TestEgress(), Update(), Deparse()) main;
