#include <core.p4>
#include <ubpf_model.p4>

header Ethernet {
    bit<48> destination;
    bit<48> source;
    bit<16> etherType;
}

header IPv4 {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> checksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

struct Headers_t {
    Ethernet ethernet;
    IPv4     ipv4;
    udp_t    udp;
}

struct metadata {
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            default: reject;
        }
    }
    state ipv4 {
        p.extract(headers.ipv4);
        transition parse_udp;
    }
    state parse_udp {
        p.extract(headers.udp);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    apply {
        bit<32> old_addr = headers.ipv4.dstAddr;
        bit<32> new_addr = 32w0x1020304;
        headers.ipv4.dstAddr = new_addr;
        headers.ipv4.checksum = csum_replace4(headers.ipv4.checksum, old_addr, new_addr);
        bit<16> from = headers.udp.dstPort;
        bit<16> to = 16w0x400;
        headers.udp.dstPort = to;
        headers.udp.checksum = csum_replace2(headers.udp.checksum, from, to);
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
        packet.emit(headers.udp);
    }
}

ubpf(prs(), pipe(), dprs()) main;
