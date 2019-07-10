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
    S base;
    U u;
}

struct metadata {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<S>(hdr.base);
        transition select(hdr.base.t) {
            8w0: parseO1;
            8w1: parseO2;
            default: accept;
        }
    }
    state parseO1 {
        packet.extract<O1>(hdr.u.byte);
        transition accept;
    }
    state parseO2 {
        packet.extract<O2>(hdr.u.short);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    table debug_hdr {
        key = {
            hdr.base.t           : exact @name("hdr.base.t") ;
            hdr.u.short.isValid(): exact @name("hdr.u.short.$valid$") ;
            hdr.u.byte.isValid() : exact @name("hdr.u.byte.$valid$") ;
        }
        actions = {
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        debug_hdr.apply();
        if (hdr.u.short.isValid()) {
            hdr.u.short.data = 16w0xffff;
        } else if (hdr.u.byte.isValid()) {
            hdr.u.byte.data = 8w0xff;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>(hdr);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

