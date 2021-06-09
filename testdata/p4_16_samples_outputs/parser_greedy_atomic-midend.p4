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

struct Headers {
    ethernet_t eth_hdr;
    ipv4_t     ipv4_hdr;
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
        transition parse_ipv4;
    }
    @name("p.a") state parse_ipv4 {
        pkt.extract_atomic<ipv4_t>(hdr.ipv4_hdr);
        transition parse_b;
    }
    @name("p.b") state parse_b {
        s.b = 8w4;
        s.c = 8w4;
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
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<ipv4_t>(h.ipv4_hdr);
    }
}

V1Switch<Headers, S>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

