#include <core.p4>
#include <v1model.p4>

struct ingress_metadata_t {
    bit<16> bd;
    bit<12> vrf;
    bit<12> v6_vrf;
    bit<1>  ipv4_term;
    bit<1>  ipv6_term;
    bit<1>  igmp_snoop;
    bit<1>  tunnel_term;
    bit<32> tunnel_vni;
    bit<16> ing_meter;
    bit<16> i_lif;
    bit<16> i_tunnel_lif;
    bit<16> o_lif;
    bit<16> if_acl_label;
    bit<16> route_acl_label;
    bit<16> bd_flags;
    bit<8>  stp_instance;
    bit<1>  route;
    bit<1>  inner_route;
    bit<1>  l2_miss;
    bit<1>  l3_lpm_miss;
    bit<16> mc_index;
    bit<16> inner_mc_index;
    bit<16> nhop;
    bit<10> ecmp_index;
    bit<14> ecmp_offset;
    bit<16> nsh_value;
    bit<8>  lag_index;
    bit<15> lag_port;
    bit<14> lag_offset;
    bit<1>  flood;
    bit<1>  learn_type;
    bit<48> learn_mac;
    bit<32> learn_ipv4;
    bit<1>  mcast_drop;
    bit<1>  drop_2;
    bit<1>  drop_1;
    bit<1>  drop_0;
    bit<8>  drop_reason;
    bit<1>  copy_to_cpu;
    bit<10> mirror_sesion_id;
    bit<1>  urpf;
    bit<2>  urpf_mode;
    bit<16> urpf_strict;
    bit<1>  ingress_bypass;
    bit<24> ipv4_dstaddr_24b;
    bit<16> nhop_index;
    bit<1>  if_drop;
    bit<1>  route_drop;
    bit<32> ipv4_dest;
    bit<48> eth_dstAddr;
    bit<48> eth_srcAddr;
    bit<8>  ipv4_ttl;
    bit<3>  dcsp;
    bit<3>  buffer_qos;
    bit<16> ip_srcPort;
    bit<16> ip_dstPort;
    bit<1>  mcast_pkt;
    bit<1>  inner_mcast_pkt;
    bit<1>  if_copy_to_cpu;
    bit<16> if_nhop;
    bit<9>  if_port;
    bit<10> if_ecmp_index;
    bit<8>  if_lag_index;
    bit<1>  route_copy_to_cpu;
    bit<16> route_nhop;
    bit<9>  route_port;
    bit<10> route_ecmp_index;
    bit<8>  route_lag_index;
    bit<8>  tun_type;
    bit<1>  nat_enable;
    bit<13> nat_source_index;
    bit<13> nat_dest_index;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".ingress_metadata") 
    ingress_metadata_t ingress_metadata;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".no_action") action no_action() {
    }
    @name(".ing_meter_set") action ing_meter_set(bit<16> meter_) {
        meta.ingress_metadata.ing_meter = meter_;
    }
    @name(".storm_control") table storm_control_0 {
        actions = {
            no_action();
            ing_meter_set();
            @defaultonly NoAction_0();
        }
        key = {
            meta.ingress_metadata.bd: exact @name("ingress_metadata.bd") ;
            hdr.ethernet.dstAddr    : ternary @name("ethernet.dstAddr") ;
        }
        size = 8192;
        default_action = NoAction_0();
    }
    apply {
        storm_control_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

