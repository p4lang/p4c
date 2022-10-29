#include <core.p4>
#include <pna.p4>

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

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, in pna_main_parser_input_metadata_t istd) {
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

bool checkValid(in headers hdr) {
    if (hdr.base.isValid() && hdr.u.short.isValid()) {
        return true;
    }
    return false;
}
control ingress(inout headers hdr, inout metadata meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    table debug_hdr {
        key = {
            hdr.base.t           : exact @name("hdr.base.t");
            hdr.u.short.isValid(): exact @name("hdr.u.short.$valid$");
            hdr.u.byte.isValid() : exact @name("hdr.u.byte.$valid$");
        }
        actions = {
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        debug_hdr.apply();
        if (checkValid(hdr)) {
            hdr.base.t = 8w3;
        }
        if (hdr.u.short.isValid()) {
            hdr.u.short.data = 16w0xffff;
        } else if (hdr.u.byte.isValid()) {
            hdr.u.byte.data = 8w0xff;
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr, in metadata meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit<headers>(hdr);
    }
}

control PreControlImpl(in headers hdr, inout metadata meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers, metadata, headers, metadata>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
