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
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("ingress.hu1") hu1_t hu1_0;
    @name("ingress.hu1b") hu1_t hu1b_0;
    apply {
        hu1_0.h1.setInvalid();
        hu1_0.h2.setInvalid();
        hu1b_0.h1.setInvalid();
        hu1b_0.h2.setInvalid();
        hdr.au1b[0].h1.setValid();
        hdr.au1b[0].h1 = (h1_t){f1 = hdr.ethernet.dstAddr[37:30],f2 = hdr.ethernet.dstAddr[36:29]};
        hdr.au1b[1].h2.setValid();
        hdr.au1b[1].h2 = (h2_t){f1 = hdr.ethernet.dstAddr[35:28],f2 = hdr.ethernet.dstAddr[34:27]};
        hdr.au1 = hdr.au1b;
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control PreControlImpl(in headers_t hdr, inout metadata_t meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(ParserImpl(), PreControlImpl(), ingress(), DeparserImpl()) main;
