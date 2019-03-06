#include <core.p4>
#include <v1model.p4>

header S {
    bit<8> t;
}

header O1 {
    bit<8> data;
}

header O2 {
    bit<16> data;
}

header_union U {
    O1 byte;
    O2 short;
}

struct headers {
    S    base;
    U[1] u;
}

struct metadata {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract(hdr.base);
        transition select(hdr.base.t) {
            0: parseO1;
            1: parseO2;
            default: accept;
        }
    }
    state parseO1 {
        packet.extract(hdr.u[0].byte);
        transition accept;
    }
    state parseO2 {
        packet.extract(hdr.u[0].short);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    table debug_hdr {
        key = {
            hdr.base.t              : exact;
            hdr.u[0].short.isValid(): exact;
            hdr.u[0].byte.isValid() : exact;
        }
        actions = {
            NoAction;
        }
        const default_action = NoAction();
    }
    apply {
        debug_hdr.apply();
        if (hdr.u[0].short.isValid()) {
            hdr.u[0].short.data = 0xffff;
        }
        if (hdr.u[0].byte.isValid()) {
            hdr.u[0].byte.data = 0xff;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

