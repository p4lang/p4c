#include <core.p4>
#include <ubpf_model.p4>

header IPv4_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
}

extern bit<16> compute_hash(in bit<32> srcAddr, in bit<32> dstAddr);
parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @hidden action ubpf_hash_extern53() {
        headers.ipv4.hdrChecksum = compute_hash(headers.ipv4.srcAddr, headers.ipv4.dstAddr);
    }
    @hidden table tbl_ubpf_hash_extern53 {
        actions = {
            ubpf_hash_extern53();
        }
        const default_action = ubpf_hash_extern53();
    }
    apply {
        tbl_ubpf_hash_extern53.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action ubpf_hash_extern60() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_ubpf_hash_extern60 {
        actions = {
            ubpf_hash_extern60();
        }
        const default_action = ubpf_hash_extern60();
    }
    apply {
        tbl_ubpf_hash_extern60.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
