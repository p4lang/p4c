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
    bit<16> _ingress_metadata_bd0;
    bit<12> _ingress_metadata_vrf1;
    bit<1>  _ingress_metadata_ipv4_unicast_enabled2;
    bit<1>  _ingress_metadata_ipv6_unicast_enabled3;
    bit<2>  _ingress_metadata_ipv4_multicast_mode4;
    bit<2>  _ingress_metadata_ipv6_multicast_mode5;
    bit<1>  _ingress_metadata_igmp_snooping_enabled6;
    bit<1>  _ingress_metadata_mld_snooping_enabled7;
    bit<2>  _ingress_metadata_ipv4_urpf_mode8;
    bit<2>  _ingress_metadata_ipv6_urpf_mode9;
    bit<10> _ingress_metadata_rmac_group10;
    bit<16> _ingress_metadata_bd_mrpf_group11;
    bit<16> _ingress_metadata_uuc_mc_index12;
    bit<16> _ingress_metadata_umc_mc_index13;
    bit<16> _ingress_metadata_bcast_mc_index14;
    bit<16> _ingress_metadata_bd_label15;
    bit<16> _intrinsic_metadata_exclusion_id116;
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        meta._ingress_metadata_bd0 = hdr.data.f2;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".set_bd_info") action set_bd_info(bit<12> vrf, bit<10> rmac_group, bit<16> mrpf_group, bit<16> bd_label, bit<16> uuc_mc_index, bit<16> bcast_mc_index, bit<16> umc_mc_index, bit<1> ipv4_unicast_enabled, bit<1> ipv6_unicast_enabled, bit<2> ipv4_multicast_mode, bit<2> ipv6_multicast_mode, bit<1> igmp_snooping_enabled, bit<1> mld_snooping_enabled, bit<2> ipv4_urpf_mode, bit<2> ipv6_urpf_mode, bit<16> exclusion_id) {
        meta._ingress_metadata_vrf1 = vrf;
        meta._ingress_metadata_ipv4_unicast_enabled2 = ipv4_unicast_enabled;
        meta._ingress_metadata_ipv6_unicast_enabled3 = ipv6_unicast_enabled;
        meta._ingress_metadata_ipv4_multicast_mode4 = ipv4_multicast_mode;
        meta._ingress_metadata_ipv6_multicast_mode5 = ipv6_multicast_mode;
        meta._ingress_metadata_igmp_snooping_enabled6 = igmp_snooping_enabled;
        meta._ingress_metadata_mld_snooping_enabled7 = mld_snooping_enabled;
        meta._ingress_metadata_ipv4_urpf_mode8 = ipv4_urpf_mode;
        meta._ingress_metadata_ipv6_urpf_mode9 = ipv6_urpf_mode;
        meta._ingress_metadata_rmac_group10 = rmac_group;
        meta._ingress_metadata_bd_mrpf_group11 = mrpf_group;
        meta._ingress_metadata_uuc_mc_index12 = uuc_mc_index;
        meta._ingress_metadata_umc_mc_index13 = umc_mc_index;
        meta._ingress_metadata_bcast_mc_index14 = bcast_mc_index;
        meta._ingress_metadata_bd_label15 = bd_label;
        meta._intrinsic_metadata_exclusion_id116 = exclusion_id;
    }
    @name(".bd") table bd_0 {
        actions = {
            set_bd_info();
            @defaultonly NoAction_0();
        }
        key = {
            meta._ingress_metadata_bd0: exact @name("ingress_metadata.bd") ;
        }
        size = 16384;
        default_action = NoAction_0();
    }
    apply {
        bd_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

