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
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.values);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        bool matched = false;
        if (hdr.values.op == 0) {
            matched = hdr.values.n4 < 1;
        }
        if (hdr.values.op == 1) {
            matched = hdr.values.n12 < 1;
        }
        if (hdr.values.op == 2) {
            matched = hdr.values.n24 == -5;
        }
        if (hdr.values.op == 3) {
            matched = hdr.values.n4 == -1;
        }
        if (hdr.values.op == 4) {
            matched = hdr.values.n12 != -1;
        }
        if (hdr.values.op == 5) {
            matched = hdr.values.mac == 48w0x21122334455;
        }
        if (hdr.values.op == 6) {
            matched = hdr.values.mac < 48w0x30000000000;
        }
        if (hdr.values.op == 7) {
            matched = hdr.values.n24 > -6;
        }
        if (!matched) {
            drop_packet();
            return;
        }
        send_to_port((PortId_t)1);
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
    }
}

PNA_NIC(Parser(), Ingress(), Deparser()) main;
