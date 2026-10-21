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
    apply {
        matched_0 = false;
        if (hdr.values.op == 8w0) {
            matched_0 = hdr.values.n4 < 4s1;
        }
        if (hdr.values.op == 8w1) {
            matched_0 = hdr.values.n12 < 12s1;
        }
        if (hdr.values.op == 8w2) {
            matched_0 = hdr.values.n24 == -24s5;
        }
        if (hdr.values.op == 8w3) {
            matched_0 = hdr.values.n4 == -4s1;
        }
        if (hdr.values.op == 8w4) {
            matched_0 = hdr.values.n12 != -12s1;
        }
        if (hdr.values.op == 8w5) {
            matched_0 = hdr.values.mac == 48w0x21122334455;
        }
        if (hdr.values.op == 8w6) {
            matched_0 = hdr.values.mac < 48w0x30000000000;
        }
        if (hdr.values.op == 8w7) {
            matched_0 = hdr.values.n24 > -24s6;
        }
        if (matched_0) {
            send_to_port((PortId_t)32w1);
        } else {
            drop_packet();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;
