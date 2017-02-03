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
    ethernet_t tmp_4_ether;
    vlan_t tmp_4_vlan;
    ipv4_t tmp_4_ipv4;
    ethernet_t tmp_5_ether;
    vlan_t tmp_5_vlan;
    ipv4_t tmp_5_ipv4;
    ethernet_t hdr_2_ether;
    vlan_t hdr_2_vlan;
    ipv4_t hdr_2_ipv4;
    ethernet_t tmp_6;
    ethernet_t ether_1;
    ethernet_t hdr_3_ether;
    vlan_t hdr_3_vlan;
    ipv4_t hdr_3_ipv4;
    bit<16> etherType_1;
    ipv4_t tmp_7;
    vlan_t tmp_8;
    ipv4_t ipv4_1;
    vlan_t vlan_1;
    state start {
        hdr_2_ether.setInvalid();
        hdr_2_vlan.setInvalid();
        hdr_2_ipv4.setInvalid();
        ether_1.setInvalid();
        b.extract<ethernet_t>(ether_1);
        tmp_6 = ether_1;
        hdr_2_ether = tmp_6;
        tmp_4_ether = hdr_2_ether;
        tmp_4_vlan = hdr_2_vlan;
        tmp_4_ipv4 = hdr_2_ipv4;
        hdr.ether = tmp_4_ether;
        hdr.vlan = tmp_4_vlan;
        hdr.ipv4 = tmp_4_ipv4;
        tmp_5_ether = hdr.ether;
        tmp_5_vlan = hdr.vlan;
        tmp_5_ipv4 = hdr.ipv4;
        hdr_3_ether = tmp_5_ether;
        hdr_3_vlan = tmp_5_vlan;
        hdr_3_ipv4 = tmp_5_ipv4;
        transition L3_start;
    }
    state L3_start {
        etherType_1 = hdr_3_ether.etherType;
        transition select(etherType_1) {
            16w0x800: L3_ipv4;
            16w0x8100: L3_vlan;
            default: start_2;
        }
    }
    state L3_ipv4 {
        ipv4_1.setInvalid();
        b.extract<ipv4_t>(ipv4_1);
        tmp_7 = ipv4_1;
        hdr_3_ipv4 = tmp_7;
        transition start_2;
    }
    state L3_vlan {
        vlan_1.setInvalid();
        b.extract<vlan_t>(vlan_1);
        tmp_8 = vlan_1;
        hdr_3_vlan = tmp_8;
        transition L3_start;
    }
    state start_2 {
        tmp_5_ether = hdr_3_ether;
        tmp_5_vlan = hdr_3_vlan;
        tmp_5_ipv4 = hdr_3_ipv4;
        hdr.ether = tmp_5_ether;
        hdr.vlan = tmp_5_vlan;
        hdr.ipv4 = tmp_5_ipv4;
        transition accept;
    }
}

control MyVerifyChecksum(in h hdr, inout m meta) {
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
