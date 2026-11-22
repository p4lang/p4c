#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
typedef bit<16> ether_type_t;
const vlan_id_t INTERNAL_VLAN_ID = 12w0xfff;
const vlan_id_t NO_VLAN_ID = 12w0x0;
header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    ether_type_t    ether_type;
}

header vlan_t {
    bit<3>       priority_code_point;
    bit<1>       drop_eligible_indicator;
    vlan_id_t    vlan_id;
    ether_type_t ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     total_len;
    bit<16>     identification;
    bit<1>      reserved;
    bit<1>      do_not_fragment;
    bit<1>      more_fragments;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     header_checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header ipv6_t {
    bit<4>      version;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<20>     flow_label;
    bit<16>     payload_length;
    bit<8>      next_header;
    bit<8>      hop_limit;
    ipv6_addr_t src_addr;
    ipv6_addr_t dst_addr;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> hdr_length;
    bit<16> checksum;
}

header tcp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header icmp_t {
    bit<8>  type;
    bit<8>  code;
    bit<16> checksum;
    bit<32> rest_of_header;
}

header arp_t {
    bit<16> hw_type;
    bit<16> proto_type;
    bit<8>  hw_addr_len;
    bit<8>  proto_addr_len;
    bit<16> opcode;
    bit<48> sender_hw_addr;
    bit<32> sender_proto_addr;
    bit<48> target_hw_addr;
    bit<32> target_proto_addr;
}

header gre_t {
    bit<1>  checksum_present;
    bit<1>  routing_present;
    bit<1>  key_present;
    bit<1>  sequence_present;
    bit<1>  strict_source_route;
    bit<3>  recursion_control;
    bit<1>  acknowledgement_present;
    bit<4>  flags;
    bit<3>  version;
    bit<16> protocol;
}

header ipfix_t {
    bit<16> version_number;
    bit<16> length;
    bit<32> export_time;
    bit<32> sequence_number;
    bit<32> observation_domain_id;
}

header psamp_extended_t {
    bit<16> template_id;
    bit<16> length;
    bit<64> observation_time;
    bit<16> flowset;
    bit<16> next_hop_index;
    bit<16> epoch;
    bit<16> ingress_port;
    bit<16> egress_port;
    bit<16> user_meta_field;
    bit<8>  dlb_id;
    bit<8>  variable_length;
    bit<16> packet_sampled_length;
}

enum bit<8> PreservedFieldList {
    MIRROR_AND_PACKET_IN_COPY = 8w1,
    RECIRCULATE = 8w2
}

type bit<10> nexthop_id_t;
type bit<10> tunnel_id_t;
type bit<12> wcmp_group_id_t;
@p4runtime_translation_mappings({ { "" , 0 } , }) type bit<10> vrf_id_t;
const vrf_id_t kDefaultVrf = (vrf_id_t)10w0;
type bit<10> router_interface_id_t;
type bit<9> port_id_t;
type bit<10> mirror_session_id_t;
type bit<8> qos_queue_t;
typedef bit<6> route_metadata_t;
typedef bit<8> acl_metadata_t;
typedef bit<16> multicast_group_id_t;
typedef bit<16> replica_instance_t;
enum bit<2> MeterColor_t {
    GREEN = 2w0,
    YELLOW = 2w1,
    RED = 2w2
}

@controller_header("packet_in") header packet_in_header_t {
    @id(1)
    port_id_t ingress_port;
    @id(2)
    port_id_t target_egress_port;
}

@controller_header("packet_out") header packet_out_header_t {
    @id(1)
    port_id_t egress_port;
    @id(2)
    bit<1>    submit_to_ingress;
    @id(3) @padding
    bit<6>    unused_pad;
}

struct headers_t {
    packet_out_header_t packet_out_header;
    ethernet_t          mirror_encap_ethernet;
    vlan_t              mirror_encap_vlan;
    ipv6_t              mirror_encap_ipv6;
    udp_t               mirror_encap_udp;
    ipfix_t             ipfix;
    psamp_extended_t    psamp_extended;
    ethernet_t          ethernet;
    vlan_t              vlan;
    ipv6_t              tunnel_encap_ipv6;
    gre_t               tunnel_encap_gre;
    ipv4_t              ipv4;
    ipv6_t              ipv6;
    ipv4_t              inner_ipv4;
    ipv6_t              inner_ipv6;
    icmp_t              icmp;
    tcp_t               tcp;
    udp_t               udp;
    arp_t               arp;
}

struct packet_rewrites_t {
    ethernet_addr_t src_mac;
    ethernet_addr_t dst_mac;
    vlan_id_t       vlan_id;
}

struct local_metadata_t {
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    bool                enable_vlan_checks;
    vlan_id_t           vlan_id;
    bool                admit_to_l3;
    vrf_id_t            vrf_id;
    bool                enable_decrement_ttl;
    bool                enable_src_mac_rewrite;
    bool                enable_dst_mac_rewrite;
    bool                enable_vlan_rewrite;
    packet_rewrites_t   packet_rewrites;
    bit<16>             l4_src_port;
    bit<16>             l4_dst_port;
    bit<8>              wcmp_selector_input;
    bool                apply_tunnel_decap_at_end_of_pre_ingress;
    bool                apply_tunnel_encap_at_egress;
    ipv6_addr_t         tunnel_encap_src_ipv6;
    ipv6_addr_t         tunnel_encap_dst_ipv6;
    bool                marked_to_copy;
    bool                marked_to_mirror;
    mirror_session_id_t mirror_session_id;
    port_id_t           mirror_egress_port;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    ethernet_addr_t     mirror_encap_src_mac;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    ethernet_addr_t     mirror_encap_dst_mac;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    vlan_id_t           mirror_encap_vlan_id;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    ipv6_addr_t         mirror_encap_src_ip;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    ipv6_addr_t         mirror_encap_dst_ip;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    bit<16>             mirror_encap_udp_src_port;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    bit<16>             mirror_encap_udp_dst_port;
    @field_list(PreservedFieldList.RECIRCULATE)
    bit<9>              loopback_port;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    bit<9>              packet_in_ingress_port;
    @field_list(PreservedFieldList.MIRROR_AND_PACKET_IN_COPY)
    bit<9>              packet_in_target_egress_port;
    MeterColor_t        color;
    port_id_t           ingress_port;
    route_metadata_t    route_metadata;
    acl_metadata_t      acl_metadata;
    bool                bypass_ingress;
    bool                wcmp_group_id_valid;
    wcmp_group_id_t     wcmp_group_id_value;
    bool                nexthop_id_valid;
    nexthop_id_t        nexthop_id_value;
    bool                ipmc_table_hit;
    bool                acl_drop;
}

control acl_wbb_ingress(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bit<8> ttl = 8w0;
    @id(0x15000101) direct_meter<MeterColor_t>(MeterType.bytes) acl_wbb_ingress_meter;
    @id(0x13000103) direct_counter(CounterType.packets_and_bytes) acl_wbb_ingress_counter;
    @id(0x01000107) @sai_action(SAI_PACKET_ACTION_COPY) action acl_wbb_ingress_copy() {
        acl_wbb_ingress_meter.read(local_metadata.color);
        clone(CloneType.I2E, 32w255);
        acl_wbb_ingress_counter.count();
    }
    @id(0x01000108) @sai_action(SAI_PACKET_ACTION_TRAP) action acl_wbb_ingress_trap() {
        acl_wbb_ingress_meter.read(local_metadata.color);
        clone(CloneType.I2E, 32w255);
        mark_to_drop(standard_metadata);
        acl_wbb_ingress_counter.count();
    }
    @p4runtime_role("sdn_controller") @id(0x02000103) @sai_acl(INGRESS) @entry_restriction("
    // WBB only allows for very specific table entries:

    // Traceroute (6 entries)
    (
      // IPv4 or IPv6
      ((is_ipv4 == 1 && is_ipv6::mask == 0) ||
        (is_ipv4::mask == 0 && is_ipv6 == 1)) &&
      // TTL 0, 1, and 2
      (ttl == 0 || ttl == 1 || ttl == 2) &&
      ether_type::mask == 0 && outer_vlan_id::mask == 0
    ) ||
    // LLDP
    (
      ether_type == 0x88cc &&
      is_ipv4::mask == 0 && is_ipv6::mask == 0 && ttl::mask == 0 &&
      outer_vlan_id::mask == 0
    ) ||
    // ND
    (
    // TODO remove optional match for VLAN ID once VLAN ID is
    // completely removed from ND flows.
      (( outer_vlan_id::mask == 0xfff && outer_vlan_id == 0x0FA0) ||
      outer_vlan_id::mask == 0);
      ether_type == 0x6007;
      is_ipv4::mask == 0;
      is_ipv6::mask == 0;
      ttl::mask == 0
    )
  ") table acl_wbb_ingress_table {
        key = {
            headers.ipv4.isValid()            : optional @name("is_ipv4") @id(1) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()            : optional @name("is_ipv6") @id(2) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type       : ternary @name("ether_type") @id(3) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            ttl                               : ternary @name("ttl") @id(4) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            headers.ipv4.header_checksum[11:0]: ternary @name("outer_vlan_id") @id(5) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID);
        }
        actions = {
            @proto_id(1) acl_wbb_ingress_copy();
            @proto_id(2) acl_wbb_ingress_trap();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        meters = acl_wbb_ingress_meter;
        counters = acl_wbb_ingress_counter;
        size = 8;
    }
    apply {
        if (headers.ipv4.isValid()) {
            ttl = headers.ipv4.ttl;
        } else if (headers.ipv6.isValid()) {
            ttl = headers.ipv6.hop_limit;
        }
        acl_wbb_ingress_table.apply();
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("acl_wbb_ingress") acl_wbb_ingress() acl_wbb_ingress_inst;
    apply {
        acl_wbb_ingress_inst.apply(headers, local_metadata, standard_metadata);
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        transition accept;
    }
}

control packet_deparser(packet_out packet, in headers_t headers) {
    apply {
    }
}

control verify_ipv4_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

control compute_ipv4_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
    }
}

@pkginfo(name="wbb.p4", organization="Google", version="0.0.0") V1Switch<headers_t, local_metadata_t>(packet_parser(), verify_ipv4_checksum(), ingress(), egress(), compute_ipv4_checksum(), packet_deparser()) main;
