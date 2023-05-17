#include <core.p4>
#include <dpdk/pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

header h1_t {
    bit<8> f1;
    bit<8> f2;
}

header h2_t {
    bit<8> f1;
    bit<8> f2;
}

header_union hu1_t {
    h1_t h1;
    h2_t h2;
}

struct metadata_t {
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout metadata_t m, in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    h1_t hu1_0_h1;
    h2_t hu1_0_h2;
    h1_t hu1b_0_h1;
    h2_t hu1b_0_h2;
    @name("ingress.a1") h1_t[2] a1_0;
    @name("ingress.a1b") h1_t[2] a1b_0;
    @name("ingress.au1") hu1_t[2] au1_0;
    @name("ingress.au1b") hu1_t[2] au1b_0;
    @hidden action pnadpdkwrongwarning50() {
        hu1_0_h1.setInvalid();
        hu1_0_h2.setInvalid();
        hu1b_0_h1.setInvalid();
        hu1b_0_h2.setInvalid();
        a1_0[0].setInvalid();
        a1_0[1].setInvalid();
        a1b_0[0].setInvalid();
        a1b_0[1].setInvalid();
        au1_0[0].h1.setInvalid();
        au1_0[0].h2.setInvalid();
        au1_0[1].h1.setInvalid();
        au1_0[1].h2.setInvalid();
        au1b_0[0].h1.setInvalid();
        au1b_0[0].h2.setInvalid();
        au1b_0[1].h1.setInvalid();
        au1b_0[1].h2.setInvalid();
    }
    @hidden table tbl_pnadpdkwrongwarning50 {
        actions = {
            pnadpdkwrongwarning50();
        }
        const default_action = pnadpdkwrongwarning50();
    }
    apply {
        tbl_pnadpdkwrongwarning50.apply();
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkwrongwarning90() {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_pnadpdkwrongwarning90 {
        actions = {
            pnadpdkwrongwarning90();
        }
        const default_action = pnadpdkwrongwarning90();
    }
    apply {
        tbl_pnadpdkwrongwarning90.apply();
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
