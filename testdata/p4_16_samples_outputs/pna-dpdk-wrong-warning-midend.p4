#include <core.p4>
#include <dpdk/pna.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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

struct headers_t {
    ethernet_t ethernet;
    hu1_t[2]   au1;
    hu1_t[2]   au1b;
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
    @hidden action pnadpdkwrongwarning53() {
        hu1_0_h1.setInvalid();
        hu1_0_h2.setInvalid();
        hu1b_0_h1.setInvalid();
        hu1b_0_h2.setInvalid();
        {
            hdr.au1b[0].h1.setValid();
            hdr.au1b[0].h2.setInvalid();
        }
        hdr.au1b[0].h1.f1 = hdr.ethernet.dstAddr[37:30];
        hdr.au1b[0].h1.f2 = hdr.ethernet.dstAddr[36:29];
        {
            hdr.au1b[1].h2.setValid();
            hdr.au1b[1].h1.setInvalid();
        }
        hdr.au1b[1].h2.f1 = hdr.ethernet.dstAddr[35:28];
        hdr.au1b[1].h2.f2 = hdr.ethernet.dstAddr[34:27];
        hdr.au1 = hdr.au1b;
    }
    @hidden table tbl_pnadpdkwrongwarning53 {
        actions = {
            pnadpdkwrongwarning53();
        }
        const default_action = pnadpdkwrongwarning53();
    }
    apply {
        tbl_pnadpdkwrongwarning53.apply();
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnadpdkwrongwarning93() {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_pnadpdkwrongwarning93 {
        actions = {
            pnadpdkwrongwarning93();
        }
        const default_action = pnadpdkwrongwarning93();
    }
    apply {
        tbl_pnadpdkwrongwarning93.apply();
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
