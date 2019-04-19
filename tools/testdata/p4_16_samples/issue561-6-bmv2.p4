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
    U[2] u;
}

struct metadata {}

parser ParserImpl(packet_in packet,
                  out headers hdr,
                  inout metadata meta,
                  inout standard_metadata_t standard_metadata)
{
    state start {
        packet.extract(hdr.base);
        transition select(hdr.base.t) {
            0: parseO1O1;
            1: parseO1O2;
            2: parseO2O1;
            3: parseO2O2;
            default: accept;
        }
    }
    state parseO1O1 {
        packet.extract(hdr.u.next.byte);
        packet.extract(hdr.u.next.byte);
        transition accept;
    }
    state parseO1O2 {
        packet.extract(hdr.u.next.byte);
        packet.extract(hdr.u.next.short);
        transition accept;
    }
    state parseO2O1 {
        packet.extract(hdr.u.next.short);
        packet.extract(hdr.u.next.byte);
        transition accept;
    }
    state parseO2O2 {
        packet.extract(hdr.u.next.short);
        packet.extract(hdr.u.next.short);
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata meta,
                inout standard_metadata_t standard_metadata) {
    table debug_hdr {
        key = {
            hdr.base.t: exact;
            hdr.u[0].byte.isValid(): exact;
            hdr.u[0].short.isValid(): exact;
            hdr.u[1].byte.isValid(): exact;
            hdr.u[1].short.isValid(): exact;
        }
        actions = { NoAction; }
        const default_action = NoAction();
    }
    apply {
        debug_hdr.apply();
        if (hdr.u[0].short.isValid()) {
            hdr.u[0].short.data = 0xFFFF;
        }
        if (hdr.u[0].byte.isValid()) {
            hdr.u[0].byte.data = 0xAA;
        }
        if (hdr.u[1].short.isValid()) {
            hdr.u[1].short.data = 0xFFFF;
        }
        if (hdr.u[1].byte.isValid()) {
            hdr.u[1].byte.data = 0xAA;
        }
    }
}

control egress(inout headers hdr,
               inout metadata meta,
               inout standard_metadata_t standard_metadata)
{ apply {} }

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply { }
}

V1Switch(ParserImpl(),
         verifyChecksum(),
         ingress(),
         egress(),
         computeChecksum(),
         DeparserImpl()) main;
