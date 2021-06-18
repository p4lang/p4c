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
    bit<160> tmp;
    bit<320> tmp_0;
    state start {
        pkt.extract_greedy<ethernet_t>(hdr.eth_hdr);
        s.start = 8w0;
        transition select(hdr.eth_hdr.eth_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: noMatch;
        }
    }
    state parse_ipv4 {
        tmp = pkt.lookahead_greedy<bit<160>>();
        hdr.ipv4_hdr.setValid();
        hdr.ipv4_hdr.version = tmp[159:156];
        hdr.ipv4_hdr.ihl = tmp[155:152];
        hdr.ipv4_hdr.diffserv = tmp[151:144];
        hdr.ipv4_hdr.total_len = tmp[143:128];
        hdr.ipv4_hdr.identification = tmp[127:112];
        hdr.ipv4_hdr.flags = tmp[111:109];
        hdr.ipv4_hdr.frag_offset = tmp[108:96];
        hdr.ipv4_hdr.ttl = tmp[95:88];
        hdr.ipv4_hdr.protocol = tmp[87:80];
        hdr.ipv4_hdr.hdr_checksum = tmp[79:64];
        hdr.ipv4_hdr.src_addr = tmp[63:32];
        hdr.ipv4_hdr.dst_addr = tmp[31:0];
        transition accept;
    }
    state parse_ipv6 {
        tmp_0 = pkt.lookahead_atomic<bit<320>>();
        hdr.ipv6.setValid();
        hdr.ipv6.version = tmp_0[319:316];
        hdr.ipv6.traffic_class = tmp_0[315:308];
        hdr.ipv6.flow_label = tmp_0[307:288];
        hdr.ipv6.payload_len = tmp_0[287:272];
        hdr.ipv6.next_hdr = tmp_0[271:264];
        hdr.ipv6.hop_limit = tmp_0[263:256];
        hdr.ipv6.src_addr = tmp_0[255:128];
        hdr.ipv6.dst_addr = tmp_0[127:0];
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
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
        pkt.emit<ipv6_h>(h.ipv6);
    }
}

V1Switch<Headers, S>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

