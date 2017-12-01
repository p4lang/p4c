#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<16> bd;
    bit<12> vrf;
    bit<1>  ipv4_unicast_enabled;
    bit<1>  ipv6_unicast_enabled;
    bit<2>  ipv4_multicast_mode;
    bit<2>  ipv6_multicast_mode;
    bit<1>  igmp_snooping_enabled;
    bit<1>  mld_snooping_enabled;
    bit<2>  ipv4_urpf_mode;
    bit<2>  ipv6_urpf_mode;
    bit<10> rmac_group;
    bit<16> bd_mrpf_group;
    bit<16> uuc_mc_index;
    bit<16> umc_mc_index;
    bit<16> bcast_mc_index;
    bit<16> bd_label;
}

struct intrinsic_metadata_t {
    bit<16> exclusion_id1;
}

header data_t {
    bit<16> f1;
    bit<16> f2;
}

struct metadata {
    @name(".ingress_metadata") 
    ingress_metadata_t   ingress_metadata;
    @name(".intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract(hdr.data);
        meta.ingress_metadata.bd = hdr.data.f2;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".set_bd_info") action set_bd_info(bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<16> exclusion_id) {
        meta.ingress_metadata.vrf = vrf;
        meta.ingress_metadata.ipv4_unicast_enabled = ipv4_unicast_enabled;
        meta.ingress_metadata.ipv6_unicast_enabled = ipv6_unicast_enabled;
        meta.ingress_metadata.ipv4_multicast_mode = ipv4_multicast_mode;
        meta.ingress_metadata.ipv6_multicast_mode = ipv6_multicast_mode;
        meta.ingress_metadata.igmp_snooping_enabled = igmp_snooping_enabled;
        meta.ingress_metadata.mld_snooping_enabled = mld_snooping_enabled;
        meta.ingress_metadata.ipv4_urpf_mode = ipv4_urpf_mode;
        meta.ingress_metadata.ipv6_urpf_mode = ipv6_urpf_mode;
        meta.ingress_metadata.rmac_group = rmac_group;
        meta.ingress_metadata.bd_mrpf_group = mrpf_group;
        meta.ingress_metadata.uuc_mc_index = uuc_mc_index;
        meta.ingress_metadata.umc_mc_index = umc_mc_index;
        meta.ingress_metadata.bcast_mc_index = bcast_mc_index;
        meta.ingress_metadata.bd_label = bd_label;
        meta.intrinsic_metadata.exclusion_id1 = exclusion_id;
    }
    @name(".bd") table bd {
        actions = {
            set_bd_info;
        }
        key = {
            meta.ingress_metadata.bd: exact;
        }
        size = 16384;
    }
    apply {
        bd.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.data);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

