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
    U[2] u;
}

struct metadata {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<S>(hdr.base);
        transition select(hdr.base.t) {
            8w0: parseO1O1;
            8w1: parseO1O2;
            8w2: parseO2O1;
            8w3: parseO2O2;
            default: accept;
        }
    }
    state parseO1O1 {
        packet.extract<O1>(hdr.u.next.byte);
        packet.extract<O1>(hdr.u.next.byte);
        transition accept;
    }
    state parseO1O2 {
        packet.extract<O1>(hdr.u.next.byte);
        packet.extract<O2>(hdr.u.next.short);
        transition accept;
    }
    state parseO2O1 {
        packet.extract<O2>(hdr.u.next.short);
        packet.extract<O1>(hdr.u.next.byte);
        transition accept;
    }
    state parseO2O2 {
        packet.extract<O2>(hdr.u.next.short);
        packet.extract<O2>(hdr.u.next.short);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.debug_hdr") table debug_hdr_0 {
        key = {
            hdr.base.t              : exact @name("hdr.base.t") ;
            hdr.u[0].byte.isValid() : exact @name("hdr.u[0].byte.$valid$") ;
            hdr.u[0].short.isValid(): exact @name("hdr.u[0].short.$valid$") ;
            hdr.u[1].byte.isValid() : exact @name("hdr.u[1].byte.$valid$") ;
            hdr.u[1].short.isValid(): exact @name("hdr.u[1].short.$valid$") ;
        }
        actions = {
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    apply {
        debug_hdr_0.apply();
        if (hdr.u[0].short.isValid()) {
            hdr.u[0].short.data = 16w0xffff;
        }
        if (hdr.u[0].byte.isValid()) {
            hdr.u[0].byte.data = 8w0xaa;
        }
        if (hdr.u[1].short.isValid()) {
            hdr.u[1].short.data = 16w0xffff;
        }
        if (hdr.u[1].byte.isValid()) {
            hdr.u[1].byte.data = 8w0xaa;
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

