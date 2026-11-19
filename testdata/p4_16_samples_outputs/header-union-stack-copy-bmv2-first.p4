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
    H    tag;
    U[2] src;
    U[2] dst;
}

struct Metadata {
}

parser Parse(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    U[2] parsed;
    state start {
        pkt.extract<H>(hdr.tag);
        transition select(hdr.tag.value) {
            8w0: first;
            8w1: second;
            default: copy;
        }
    }
    state first {
        pkt.extract<H>(parsed[0].first);
        transition copy;
    }
    state second {
        pkt.extract<H>(parsed[1].second);
        transition copy;
    }
    state copy {
        hdr.src[0] = parsed[0];
        hdr.src[1] = parsed[1];
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
    U[2] copies;
    apply {
        copies[0] = hdr.src[0];
        copies[1] = hdr.src[1];
        hdr.dst[hdr.tag.value[0:0]] = copies[hdr.tag.value[0:0]];
        sm.egress_spec = 9w1;
    }
}

control Deparse(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit<H>(hdr.tag);
        pkt.emit<U[2]>(hdr.dst);
    }
}

V1Switch<Headers, Metadata>(Parse(), Verify(), TestIngress(), TestEgress(), Update(), Deparse()) main;
