#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header H {
    bit<8> value;
    bit<8> offset;
    bit<8> result;
}

struct Headers {
    H h;
}

struct Metadata {
}

parser SliceParser(packet_in pkt, out Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    state start {
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control Verify(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control SliceIngress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    @hidden action shifteddynamicslicebmv2l33() {
        hdr.h.result = (bit<8>)(hdr.h.value << 8w4)[hdr.h.offset+:4];
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_shifteddynamicslicebmv2l33 {
        actions = {
            shifteddynamicslicebmv2l33();
        }
        const default_action = shifteddynamicslicebmv2l33();
    }
    apply {
        tbl_shifteddynamicslicebmv2l33.apply();
    }
}

control SliceEgress(inout Headers hdr, inout Metadata meta, inout standard_metadata_t sm) {
    apply {
    }
}

control Compute(inout Headers hdr, inout Metadata meta) {
    apply {
    }
}

control SliceDeparser(packet_out pkt, in Headers hdr) {
    apply {
        pkt.emit<H>(hdr.h);
    }
}

V1Switch<Headers, Metadata>(SliceParser(), Verify(), SliceIngress(), SliceEgress(), Compute(), SliceDeparser()) main;
