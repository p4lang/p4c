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

extern bit<16> incremental_checksum(in bit<16> csum, in bit<32> old, in bit<32> new);
extern bool verify_ipv4_checksum(in IPv4_h iphdr);
parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.verified") bool verified_0;
    @name("pipe.old_addr") bit<32> old_addr_0;
    @hidden action ubpf_checksum_extern56() {
        old_addr_0 = headers.ipv4.dstAddr;
        headers.ipv4.dstAddr = 32w0x1020304;
        headers.ipv4.hdrChecksum = incremental_checksum(headers.ipv4.hdrChecksum, old_addr_0, 32w0x1020304);
    }
    @hidden action ubpf_checksum_extern61() {
        mark_to_drop();
    }
    @hidden action ubpf_checksum_extern54() {
        verified_0 = verify_ipv4_checksum(headers.ipv4);
    }
    @hidden table tbl_ubpf_checksum_extern54 {
        actions = {
            ubpf_checksum_extern54();
        }
        const default_action = ubpf_checksum_extern54();
    }
    @hidden table tbl_ubpf_checksum_extern56 {
        actions = {
            ubpf_checksum_extern56();
        }
        const default_action = ubpf_checksum_extern56();
    }
    @hidden table tbl_ubpf_checksum_extern61 {
        actions = {
            ubpf_checksum_extern61();
        }
        const default_action = ubpf_checksum_extern61();
    }
    apply {
        tbl_ubpf_checksum_extern54.apply();
        if (verified_0) {
            tbl_ubpf_checksum_extern56.apply();
        } else {
            tbl_ubpf_checksum_extern61.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action ubpf_checksum_extern69() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_ubpf_checksum_extern69 {
        actions = {
            ubpf_checksum_extern69();
        }
        const default_action = ubpf_checksum_extern69();
    }
    apply {
        tbl_ubpf_checksum_extern69.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;
