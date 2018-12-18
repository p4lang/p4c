#include <core.p4>
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
    ethernet_t hdr_0_ether;
    vlan_t hdr_0_vlan;
    ipv4_t hdr_0_ipv4;
    ethernet_t l2_ether;
    ethernet_t hdr_1_ether;
    vlan_t hdr_1_vlan;
    ipv4_t hdr_1_ipv4;
    ipv4_t l3_ipv4;
    vlan_t l3_vlan;
    state start {
        hdr_0_ether.setInvalid();
        hdr_0_vlan.setInvalid();
        hdr_0_ipv4.setInvalid();
        l2_ether.setInvalid();
        b.extract<ethernet_t>(l2_ether);
        hdr_0_ether = l2_ether;
        hdr.ether = l2_ether;
        hdr.vlan = hdr_0_vlan;
        hdr.ipv4 = hdr_0_ipv4;
        hdr_1_ether = l2_ether;
        hdr_1_vlan = hdr_0_vlan;
        hdr_1_ipv4 = hdr_0_ipv4;
        transition L3_start;
    }
    state L3_start {
        transition select(hdr_1_ether.etherType) {
            16w0x800: L3_ipv4;
            16w0x8100: L3_vlan;
            default: start_2;
        }
    }
    state L3_ipv4 {
        l3_ipv4.setInvalid();
        b.extract<ipv4_t>(l3_ipv4);
        hdr_1_ipv4 = l3_ipv4;
        transition start_2;
    }
    state L3_vlan {
        l3_vlan.setInvalid();
        b.extract<vlan_t>(l3_vlan);
        hdr_1_vlan = l3_vlan;
        transition L3_start;
    }
    state start_2 {
        hdr.ether = hdr_1_ether;
        hdr.vlan = hdr_1_vlan;
        hdr.ipv4 = hdr_1_ipv4;
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

