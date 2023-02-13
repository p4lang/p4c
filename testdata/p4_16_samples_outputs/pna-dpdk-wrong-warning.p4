#include <core.p4>
#include <dpdk/pna.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
        packet.extract(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    h1_t h1;
    h1_t h1b;
    h2_t h2;
    hu1_t hu1;
    hu1_t hu1b;
    apply {
        hu1b.h1 = { hdr.ethernet.dstAddr[45:38], hdr.ethernet.dstAddr[44:37] };
        hu1 = hu1b;
        hdr.au1b[0].h1 = { hdr.ethernet.dstAddr[37:30], hdr.ethernet.dstAddr[36:29] };
        hdr.au1b[1].h2 = { hdr.ethernet.dstAddr[35:28], hdr.ethernet.dstAddr[34:27] };
        hdr.au1 = hdr.au1b;
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
