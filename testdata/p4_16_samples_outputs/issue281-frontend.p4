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

parser EthernetParser(packet_in b, out ethernet_t ether) {
    state start {
        b.extract<ethernet_t>(ether);
        transition accept;
    }
}

parser L2(packet_in b, out h hdr) {
    @name("ep") EthernetParser() ep_0;
    state start {
        ep_0.apply(b, hdr.ether);
        transition accept;
    }
}

parser Ipv4Parser(packet_in b, out ipv4_t ipv4) {
    state start {
        b.extract<ipv4_t>(ipv4);
        transition accept;
    }
}

parser VlanParser(packet_in b, out vlan_t vlan) {
    state start {
        b.extract<vlan_t>(vlan);
        transition accept;
    }
}

parser L3(packet_in b, inout h hdr) {
    bit<16> etherType_0;
    @name("ip") Ipv4Parser() ip_0;
    @name("vp") VlanParser() vp_0;
    state start {
        etherType_0 = hdr.ether.etherType;
        transition select(etherType_0) {
            16w0x800: ipv4;
            16w0x8100: vlan;
            default: accept;
        }
    }
    state ipv4 {
        ip_0.apply(b, hdr.ipv4);
        transition accept;
    }
    state vlan {
        vp_0.apply(b, hdr.vlan);
        transition start;
    }
}

parser MyParser(packet_in b, out h hdr, inout m meta, inout standard_metadata_t std) {
    @name("l2") L2() l2_0;
    @name("l3") L3() l3_0;
    state start {
        l2_0.apply(b, hdr);
        l3_0.apply(b, hdr);
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

