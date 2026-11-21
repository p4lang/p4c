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
    @hidden action tccodegencomparison33() {
        matched_0 = hdr.values.n4 < 4s1;
    }
    @hidden action tccodegencomparison32() {
        matched_0 = false;
    }
    @hidden action tccodegencomparison34() {
        matched_0 = hdr.values.n12 < 12s1;
    }
    @hidden action tccodegencomparison35() {
        matched_0 = hdr.values.n24 == -24s5;
    }
    @hidden action tccodegencomparison36() {
        matched_0 = hdr.values.n4 == -4s1;
    }
    @hidden action tccodegencomparison37() {
        matched_0 = hdr.values.n12 != -12s1;
    }
    @hidden action tccodegencomparison38() {
        matched_0 = hdr.values.mac == 48w0x21122334455;
    }
    @hidden action tccodegencomparison39() {
        matched_0 = hdr.values.mac < 48w0x30000000000;
    }
    @hidden action tccodegencomparison40() {
        matched_0 = hdr.values.n24 > -24s6;
    }
    @hidden action tccodegencomparison42() {
        send_to_port(32w1);
    }
    @hidden action tccodegencomparison41() {
        drop_packet();
    }
    @hidden table tbl_tccodegencomparison32 {
        actions = {
            tccodegencomparison32();
        }
        const default_action = tccodegencomparison32();
    }
    @hidden table tbl_tccodegencomparison33 {
        actions = {
            tccodegencomparison33();
        }
        const default_action = tccodegencomparison33();
    }
    @hidden table tbl_tccodegencomparison34 {
        actions = {
            tccodegencomparison34();
        }
        const default_action = tccodegencomparison34();
    }
    @hidden table tbl_tccodegencomparison35 {
        actions = {
            tccodegencomparison35();
        }
        const default_action = tccodegencomparison35();
    }
    @hidden table tbl_tccodegencomparison36 {
        actions = {
            tccodegencomparison36();
        }
        const default_action = tccodegencomparison36();
    }
    @hidden table tbl_tccodegencomparison37 {
        actions = {
            tccodegencomparison37();
        }
        const default_action = tccodegencomparison37();
    }
    @hidden table tbl_tccodegencomparison38 {
        actions = {
            tccodegencomparison38();
        }
        const default_action = tccodegencomparison38();
    }
    @hidden table tbl_tccodegencomparison39 {
        actions = {
            tccodegencomparison39();
        }
        const default_action = tccodegencomparison39();
    }
    @hidden table tbl_tccodegencomparison40 {
        actions = {
            tccodegencomparison40();
        }
        const default_action = tccodegencomparison40();
    }
    @hidden table tbl_tccodegencomparison42 {
        actions = {
            tccodegencomparison42();
        }
        const default_action = tccodegencomparison42();
    }
    @hidden table tbl_tccodegencomparison41 {
        actions = {
            tccodegencomparison41();
        }
        const default_action = tccodegencomparison41();
    }
    apply {
        tbl_tccodegencomparison32.apply();
        if (hdr.values.op == 8w0) {
            tbl_tccodegencomparison33.apply();
        }
        if (hdr.values.op == 8w1) {
            tbl_tccodegencomparison34.apply();
        }
        if (hdr.values.op == 8w2) {
            tbl_tccodegencomparison35.apply();
        }
        if (hdr.values.op == 8w3) {
            tbl_tccodegencomparison36.apply();
        }
        if (hdr.values.op == 8w4) {
            tbl_tccodegencomparison37.apply();
        }
        if (hdr.values.op == 8w5) {
            tbl_tccodegencomparison38.apply();
        }
        if (hdr.values.op == 8w6) {
            tbl_tccodegencomparison39.apply();
        }
        if (hdr.values.op == 8w7) {
            tbl_tccodegencomparison40.apply();
        }
        if (matched_0) {
            tbl_tccodegencomparison42.apply();
        } else {
            tbl_tccodegencomparison41.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegencomparison48() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<values_t>(hdr.values);
    }
    @hidden table tbl_tccodegencomparison48 {
        actions = {
            tccodegencomparison48();
        }
        const default_action = tccodegencomparison48();
    }
    apply {
        tbl_tccodegencomparison48.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
