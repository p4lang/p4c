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
    S hdr_1_base;
    O1 hdr_1_u_byte;
    O2 hdr_1_u_short;
    @name("ingress.retval") bool retval;
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
    @hidden action pnaexampleheaderunion1l57() {
        retval = true;
    }
    @hidden action pnaexampleheaderunion1l58() {
        retval = false;
    }
    @hidden action act() {
        hdr_1_base = hdr.base;
        if (hdr.u_byte.isValid()) {
            hdr_1_u_byte.setValid();
            hdr_1_u_byte = hdr.u_byte;
            hdr_1_u_short.setInvalid();
        } else {
            hdr_1_u_byte.setInvalid();
        }
        if (hdr.u_short.isValid()) {
            hdr_1_u_short.setValid();
            hdr_1_u_short = hdr.u_short;
            hdr_1_u_byte.setInvalid();
        } else {
            hdr_1_u_short.setInvalid();
        }
    }
    @hidden action pnaexampleheaderunion1l79() {
        hdr.base.t = 8w3;
    }
    @hidden action pnaexampleheaderunion1l81() {
        hdr.u_short.setValid();
        hdr.u_short.data = 16w0xffff;
        hdr.u_byte.setInvalid();
    }
    @hidden action pnaexampleheaderunion1l83() {
        hdr.u_byte.setValid();
        hdr.u_byte.data = 8w0xff;
        hdr.u_short.setInvalid();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_pnaexampleheaderunion1l57 {
        actions = {
            pnaexampleheaderunion1l57();
        }
        const default_action = pnaexampleheaderunion1l57();
    }
    @hidden table tbl_pnaexampleheaderunion1l58 {
        actions = {
            pnaexampleheaderunion1l58();
        }
        const default_action = pnaexampleheaderunion1l58();
    }
    @hidden table tbl_pnaexampleheaderunion1l79 {
        actions = {
            pnaexampleheaderunion1l79();
        }
        const default_action = pnaexampleheaderunion1l79();
    }
    @hidden table tbl_pnaexampleheaderunion1l81 {
        actions = {
            pnaexampleheaderunion1l81();
        }
        const default_action = pnaexampleheaderunion1l81();
    }
    @hidden table tbl_pnaexampleheaderunion1l83 {
        actions = {
            pnaexampleheaderunion1l83();
        }
        const default_action = pnaexampleheaderunion1l83();
    }
    apply {
        debug_hdr_0.apply();
        tbl_act.apply();
        if (hdr_1_base.isValid() && hdr_1_u_short.isValid()) {
            tbl_pnaexampleheaderunion1l57.apply();
        } else {
            tbl_pnaexampleheaderunion1l58.apply();
        }
        if (retval) {
            tbl_pnaexampleheaderunion1l79.apply();
        }
        if (hdr.u_short.isValid()) {
            tbl_pnaexampleheaderunion1l81.apply();
        } else if (hdr.u_byte.isValid()) {
            tbl_pnaexampleheaderunion1l83.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr, in metadata meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnaexampleheaderunion1l92() {
        packet.emit<S>(hdr.base);
        packet.emit<O1>(hdr.u_byte);
        packet.emit<O2>(hdr.u_short);
    }
    @hidden table tbl_pnaexampleheaderunion1l92 {
        actions = {
            pnaexampleheaderunion1l92();
        }
        const default_action = pnaexampleheaderunion1l92();
    }
    apply {
        tbl_pnaexampleheaderunion1l92.apply();
    }
}

control PreControlImpl(in headers hdr, inout metadata meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers, metadata, headers, metadata>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
