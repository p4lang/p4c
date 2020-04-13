#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct meta_t {
}

header eth_h {
    bit<48> dst;
    bit<48> src;
    bit<16> type;
}

header ipv4_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  tos;
    bit<16> len;
    bit<16> id;
    bit<3>  flags;
    bit<13> frag;
    bit<8>  ttl;
    bit<8>  proto;
    bit<16> chksum;
    bit<32> src;
    bit<32> dst;
}

struct headers_t {
    eth_h  eth;
    ipv4_h ipv4;
}

parser InnerParser(packet_in pkt, out headers_t hdr) {
    state start {
        transition parse_eth;
    }
    state parse_eth {
        pkt.extract(hdr.eth);
        transition select(hdr.eth.type) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }
}

parser OuterParser(packet_in pkt, out headers_t hdr, inout meta_t m, inout standard_metadata_t meta) {
    state start {
        InnerParser.apply(pkt, hdr);
        transition accept;
    }
}

control NoVerify(inout headers_t hdr, inout meta_t m) {
    apply {
    }
}

control NoCheck(inout headers_t hdr, inout meta_t m) {
    apply {
    }
}

control NoIngress(inout headers_t hdr, inout meta_t m, inout standard_metadata_t meta) {
    apply {
    }
}

control NoEgress(inout headers_t hdr, inout meta_t m, inout standard_metadata_t meta) {
    apply {
    }
}

control SimpleDeparser(packet_out pkt, in headers_t hdr) {
    apply {
        pkt.emit(hdr.eth);
        pkt.emit(hdr.ipv4);
    }
}

V1Switch(OuterParser(), NoVerify(), NoIngress(), NoEgress(), NoCheck(), SimpleDeparser()) main;

