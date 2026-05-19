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

struct headers {
    S  base;
    O1 u_byte;
    O2 u_short;
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
        packet.extract<O1>(hdr.u_byte);
        transition accept;
    }
    state parseO2 {
        packet.extract<O2>(hdr.u_short);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.debug_hdr") table debug_hdr_0 {
        key = {
            hdr.base.t           : exact @name("hdr.base.t");
            hdr.u_short.isValid(): exact @name("hdr.u.short.$valid$");
            hdr.u_byte.isValid() : exact @name("hdr.u.byte.$valid$");
        }
        actions = {
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @hidden action pnaexampleheaderunion72() {
        hdr.u_short.setValid();
        hdr.u_short.data = 16w0xffff;
        hdr.u_byte.setInvalid();
    }
    @hidden action pnaexampleheaderunion74() {
        hdr.u_byte.setValid();
        hdr.u_byte.data = 8w0xff;
        hdr.u_short.setInvalid();
    }
    @hidden table tbl_pnaexampleheaderunion72 {
        actions = {
            pnaexampleheaderunion72();
        }
        const default_action = pnaexampleheaderunion72();
    }
    @hidden table tbl_pnaexampleheaderunion74 {
        actions = {
            pnaexampleheaderunion74();
        }
        const default_action = pnaexampleheaderunion74();
    }
    apply {
        debug_hdr_0.apply();
        if (hdr.u_short.isValid()) {
            tbl_pnaexampleheaderunion72.apply();
        } else if (hdr.u_byte.isValid()) {
            tbl_pnaexampleheaderunion74.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr, in metadata meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampleheaderunion83() {
        packet.emit<S>(hdr.base);
        packet.emit<O1>(hdr.u_byte);
        packet.emit<O2>(hdr.u_short);
    }
    @hidden table tbl_pnaexampleheaderunion83 {
        actions = {
            pnaexampleheaderunion83();
        }
        const default_action = pnaexampleheaderunion83();
    }
    apply {
        tbl_pnaexampleheaderunion83.apply();
    }
}

control PreControlImpl(in headers hdr, inout metadata meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers, metadata, headers, metadata>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
