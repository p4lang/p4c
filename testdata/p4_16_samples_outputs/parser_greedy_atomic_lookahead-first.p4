#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header ipv6_h {
    bit<4>   version;
    bit<8>   traffic_class;
    bit<20>  flow_label;
    bit<16>  payload_len;
    bit<8>   next_hdr;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
}

struct Headers {
    ethernet_t eth_hdr;
    ipv4_t     ipv4_hdr;
    ipv6_h     ipv6;
}

struct S {
    bit<8> start;
    bit<8> b;
    bit<8> c;
}

parser p(packet_in pkt, out Headers hdr, inout S s, inout standard_metadata_t sm) {
    state start {
        pkt.extract_greedy<ethernet_t>(hdr.eth_hdr);
        s.start = 8w0;
        transition select(hdr.eth_hdr.eth_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
        }
    }
    state parse_ipv4 {
        hdr.ipv4_hdr = pkt.lookahead_greedy<ipv4_t>();
        transition accept;
    }
    state parse_ipv6 {
        hdr.ipv6 = pkt.lookahead_atomic<ipv6_h>();
        transition accept;
    }
}

control ingress(inout Headers h, inout S s, inout standard_metadata_t sm) {
    apply {
    }
}

control vrfy(inout Headers h, inout S s) {
    apply {
    }
}

control update(inout Headers h, inout S s) {
    apply {
    }
}

control egress(inout Headers h, inout S s, inout standard_metadata_t sm) {
    apply {
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<Headers>(h);
    }
}

V1Switch<Headers, S>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

