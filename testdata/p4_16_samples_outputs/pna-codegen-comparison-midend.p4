#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header values_t {
    bit<8>  op;
    int<12> n12;
    int<4>  n4;
    int<24> n24;
    @tc_type("macaddr")
    bit<48> mac;
}

struct headers_t {
    ethernet_t ethernet;
    values_t   values;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<values_t>(hdr.values);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("Ingress.matched") bool matched_0;
    @hidden action pnacodegencomparison33() {
        matched_0 = hdr.values.n4 < 4s1;
    }
    @hidden action pnacodegencomparison32() {
        matched_0 = false;
    }
    @hidden action pnacodegencomparison34() {
        matched_0 = hdr.values.n12 < 12s1;
    }
    @hidden action pnacodegencomparison35() {
        matched_0 = hdr.values.n24 == -24s5;
    }
    @hidden action pnacodegencomparison36() {
        matched_0 = hdr.values.n4 == -4s1;
    }
    @hidden action pnacodegencomparison37() {
        matched_0 = hdr.values.n12 != -12s1;
    }
    @hidden action pnacodegencomparison38() {
        matched_0 = hdr.values.mac == 48w0x21122334455;
    }
    @hidden action pnacodegencomparison39() {
        matched_0 = hdr.values.mac < 48w0x30000000000;
    }
    @hidden action pnacodegencomparison40() {
        matched_0 = hdr.values.n24 > -24s6;
    }
    @hidden action pnacodegencomparison42() {
        send_to_port(32w1);
    }
    @hidden action pnacodegencomparison41() {
        drop_packet();
    }
    @hidden table tbl_pnacodegencomparison32 {
        actions = {
            pnacodegencomparison32();
        }
        const default_action = pnacodegencomparison32();
    }
    @hidden table tbl_pnacodegencomparison33 {
        actions = {
            pnacodegencomparison33();
        }
        const default_action = pnacodegencomparison33();
    }
    @hidden table tbl_pnacodegencomparison34 {
        actions = {
            pnacodegencomparison34();
        }
        const default_action = pnacodegencomparison34();
    }
    @hidden table tbl_pnacodegencomparison35 {
        actions = {
            pnacodegencomparison35();
        }
        const default_action = pnacodegencomparison35();
    }
    @hidden table tbl_pnacodegencomparison36 {
        actions = {
            pnacodegencomparison36();
        }
        const default_action = pnacodegencomparison36();
    }
    @hidden table tbl_pnacodegencomparison37 {
        actions = {
            pnacodegencomparison37();
        }
        const default_action = pnacodegencomparison37();
    }
    @hidden table tbl_pnacodegencomparison38 {
        actions = {
            pnacodegencomparison38();
        }
        const default_action = pnacodegencomparison38();
    }
    @hidden table tbl_pnacodegencomparison39 {
        actions = {
            pnacodegencomparison39();
        }
        const default_action = pnacodegencomparison39();
    }
    @hidden table tbl_pnacodegencomparison40 {
        actions = {
            pnacodegencomparison40();
        }
        const default_action = pnacodegencomparison40();
    }
    @hidden table tbl_pnacodegencomparison42 {
        actions = {
            pnacodegencomparison42();
        }
        const default_action = pnacodegencomparison42();
    }
    @hidden table tbl_pnacodegencomparison41 {
        actions = {
            pnacodegencomparison41();
        }
        const default_action = pnacodegencomparison41();
    }
    apply {
        tbl_pnacodegencomparison32.apply();
        if (hdr.values.op == 8w0) {
            tbl_pnacodegencomparison33.apply();
        }
        if (hdr.values.op == 8w1) {
            tbl_pnacodegencomparison34.apply();
        }
        if (hdr.values.op == 8w2) {
            tbl_pnacodegencomparison35.apply();
        }
        if (hdr.values.op == 8w3) {
            tbl_pnacodegencomparison36.apply();
        }
        if (hdr.values.op == 8w4) {
            tbl_pnacodegencomparison37.apply();
        }
        if (hdr.values.op == 8w5) {
            tbl_pnacodegencomparison38.apply();
        }
        if (hdr.values.op == 8w6) {
            tbl_pnacodegencomparison39.apply();
        }
        if (hdr.values.op == 8w7) {
            tbl_pnacodegencomparison40.apply();
        }
        if (matched_0) {
            tbl_pnacodegencomparison42.apply();
        } else {
            tbl_pnacodegencomparison41.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegencomparison48() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_pnacodegencomparison48 {
        actions = {
            pnacodegencomparison48();
        }
        const default_action = pnacodegencomparison48();
    }
    apply {
        tbl_pnacodegencomparison48.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
