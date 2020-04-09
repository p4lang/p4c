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
            default: skip;
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
    state skip {
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name("ingress.debug_hdr") table debug_hdr_0 {
        key = {
            hdr.base.t           : exact @name("hdr.base.t") ;
            hdr.u.short.isValid(): exact @name("hdr.u.short.$valid$") ;
            hdr.u.byte.isValid() : exact @name("hdr.u.byte.$valid$") ;
        }
        actions = {
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    @hidden action issue5613bmv2l69() {
        hdr.u.short.data = 16w0xaaaa;
        hdr.u.byte.setValid();
        hdr.u.byte.data = 8w0xff;
    }
    @hidden action issue5613bmv2l74() {
        hdr.u.byte.data = 8w0xaa;
        hdr.u.short.setValid();
        hdr.u.short.data = 16w0xffff;
    }
    @hidden table tbl_issue5613bmv2l69 {
        actions = {
            issue5613bmv2l69();
        }
        const default_action = issue5613bmv2l69();
    }
    @hidden table tbl_issue5613bmv2l74 {
        actions = {
            issue5613bmv2l74();
        }
        const default_action = issue5613bmv2l74();
    }
    apply {
        debug_hdr_0.apply();
        if (hdr.u.short.isValid()) {
            tbl_issue5613bmv2l69.apply();
        } else if (hdr.u.byte.isValid()) {
            tbl_issue5613bmv2l74.apply();
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<S>(hdr.base);
        packet.emit<O1>(hdr.u.byte);
        packet.emit<O2>(hdr.u.short);
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

