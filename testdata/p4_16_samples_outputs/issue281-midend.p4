#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
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

header vlan_t {
    bit<3>  pcp;
    bit<1>  cfi;
    bit<12> vid;
    bit<16> etherType;
}

struct h {
    ethernet_t ether;
    vlan_t     vlan;
    ipv4_t     ipv4;
}

struct m {
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    @name("MyParser.l3.etherType") bit<16> l3_etherType;
    state start {
        hdr.ether.setInvalid();
        hdr.vlan.setInvalid();
        hdr.ipv4.setInvalid();
        hdr.ether.setInvalid();
        b.extract<ethernet_t>(hdr.ether);
        l3_etherType = hdr.ether.etherType;
        transition L3_start_0;
    }
    state L3_start_0 {
        transition select(l3_etherType) {
            16w0x800: L3_ipv4;
            16w0x8100: L3_vlan;
            default: start_3;
        }
    }
    state L3_ipv4 {
        hdr.ipv4.setInvalid();
        b.extract<ipv4_t>(hdr.ipv4);
        transition start_3;
    }
    state L3_vlan {
        hdr.vlan.setInvalid();
        b.extract<vlan_t>(hdr.vlan);
        l3_etherType = hdr.vlan.etherType;
        transition L3_start_0;
    }
    state start_3 {
        transition accept;
    }
}

control MyVerifyChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyEgress(inout h hdr, inout m meta, inout standard_metadata_t std) {
    apply {
    }
}

control MyComputeChecksum(inout h hdr, inout m meta) {
    apply {
    }
}

control MyDeparser(packet_out b, in h hdr) {
    apply {
    }
}

V1Switch<h, m>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
