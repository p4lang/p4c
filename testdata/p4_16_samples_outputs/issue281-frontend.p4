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
    h hdr_2;
    ethernet_t l2_ether_0;
    h hdr_3;
    bit<16> l3_etherType_0;
    ipv4_t l3_ipv4_0;
    vlan_t l3_vlan_0;
    state start {
        hdr_2.ether.setInvalid();
        hdr_2.vlan.setInvalid();
        hdr_2.ipv4.setInvalid();
        transition L2_start;
    }
    state L2_start {
        l2_ether_0.setInvalid();
        transition L2_EthernetParser_start;
    }
    state L2_EthernetParser_start {
        b.extract<ethernet_t>(l2_ether_0);
        transition L2_start_0;
    }
    state L2_start_0 {
        hdr_2.ether = l2_ether_0;
        transition start_1;
    }
    state start_1 {
        hdr = hdr_2;
        hdr_3 = hdr;
        transition L3_start;
    }
    state L3_start {
        l3_etherType_0 = hdr_3.ether.etherType;
        transition select(l3_etherType_0) {
            16w0x800: L3_ipv4;
            16w0x8100: L3_vlan;
            default: start_2;
        }
    }
    state L3_ipv4 {
        l3_ipv4_0.setInvalid();
        transition L3_Ipv4Parser_start;
    }
    state L3_Ipv4Parser_start {
        b.extract<ipv4_t>(l3_ipv4_0);
        transition L3_ipv4_0;
    }
    state L3_ipv4_0 {
        hdr_3.ipv4 = l3_ipv4_0;
        transition start_2;
    }
    state L3_vlan {
        l3_vlan_0.setInvalid();
        transition L3_VlanParser_start;
    }
    state L3_VlanParser_start {
        b.extract<vlan_t>(l3_vlan_0);
        transition L3_vlan_0;
    }
    state L3_vlan_0 {
        hdr_3.vlan = l3_vlan_0;
        transition L3_start;
    }
    state start_2 {
        hdr = hdr_3;
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

