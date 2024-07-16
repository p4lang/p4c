#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header vlan_t {
    bit<3>  priority_code_point;
    bit<1>  drop_eligible_indicator;
    bit<12> vlan_id;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> total_len;
    bit<16> identification;
    bit<1>  reserved;
    bit<1>  do_not_fragment;
    bit<1>  more_fragments;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> header_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header ipv6_t {
    bit<4>   version;
    bit<6>   dscp;
    bit<2>   ecn;
    bit<20>  flow_label;
    bit<16>  payload_length;
    bit<8>   next_header;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
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

@controller_header("packet_in") header packet_in_header_t {
    @id(1)
    bit<9> ingress_port;
    @id(2)
    bit<9> target_egress_port;
}

@controller_header("packet_out") header packet_out_header_t {
    @id(1)
    bit<9> egress_port;
    @id(2)
    bit<1> submit_to_ingress;
    @id(3) @padding
    bit<6> unused_pad;
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
    bit<48> src_mac;
    bit<48> dst_mac;
    bit<12> vlan_id;
}

struct local_metadata_t {
    @field_list(8w1)
    bool     _enable_vlan_checks0;
    bit<12>  _vlan_id1;
    bool     _admit_to_l32;
    bit<10>  _vrf_id3;
    bool     _enable_decrement_ttl4;
    bool     _enable_src_mac_rewrite5;
    bool     _enable_dst_mac_rewrite6;
    bool     _enable_vlan_rewrite7;
    bit<48>  _packet_rewrites_src_mac8;
    bit<48>  _packet_rewrites_dst_mac9;
    bit<12>  _packet_rewrites_vlan_id10;
    bit<16>  _l4_src_port11;
    bit<16>  _l4_dst_port12;
    bit<8>   _wcmp_selector_input13;
    bool     _apply_tunnel_decap_at_end_of_pre_ingress14;
    bool     _apply_tunnel_encap_at_egress15;
    bit<128> _tunnel_encap_src_ipv616;
    bit<128> _tunnel_encap_dst_ipv617;
    bool     _marked_to_copy18;
    bool     _marked_to_mirror19;
    bit<10>  _mirror_session_id20;
    bit<9>   _mirror_egress_port21;
    @field_list(8w1)
    bit<48>  _mirror_encap_src_mac22;
    @field_list(8w1)
    bit<48>  _mirror_encap_dst_mac23;
    @field_list(8w1)
    bit<12>  _mirror_encap_vlan_id24;
    @field_list(8w1)
    bit<128> _mirror_encap_src_ip25;
    @field_list(8w1)
    bit<128> _mirror_encap_dst_ip26;
    @field_list(8w1)
    bit<16>  _mirror_encap_udp_src_port27;
    @field_list(8w1)
    bit<16>  _mirror_encap_udp_dst_port28;
    @field_list(8w2)
    bit<9>   _loopback_port29;
    @field_list(8w1)
    bit<9>   _packet_in_ingress_port30;
    @field_list(8w1)
    bit<9>   _packet_in_target_egress_port31;
    bit<2>   _color32;
    bit<9>   _ingress_port33;
    bit<6>   _route_metadata34;
    bit<8>   _acl_metadata35;
    bool     _bypass_ingress36;
    bool     _wcmp_group_id_valid37;
    bit<12>  _wcmp_group_id_value38;
    bool     _nexthop_id_valid39;
    bit<10>  _nexthop_id_value40;
    bool     _ipmc_table_hit41;
    bool     _acl_drop42;
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("ingress.acl_wbb_ingress.ttl") bit<8> acl_wbb_ingress_ttl;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @id(0x15000101) @name("ingress.acl_wbb_ingress.acl_wbb_ingress_meter") direct_meter<bit<2>>(MeterType.bytes) acl_wbb_ingress_acl_wbb_ingress_meter;
    @id(0x13000103) @name("ingress.acl_wbb_ingress.acl_wbb_ingress_counter") direct_counter(CounterType.packets_and_bytes) acl_wbb_ingress_acl_wbb_ingress_counter;
    @id(0x01000107) @sai_action(SAI_PACKET_ACTION_COPY) @name("ingress.acl_wbb_ingress.acl_wbb_ingress_copy") action acl_wbb_ingress_acl_wbb_ingress_copy_0() {
        acl_wbb_ingress_acl_wbb_ingress_meter.read(local_metadata._color32);
        clone(CloneType.I2E, 32w255);
        acl_wbb_ingress_acl_wbb_ingress_counter.count();
    }
    @id(0x01000108) @sai_action(SAI_PACKET_ACTION_TRAP) @name("ingress.acl_wbb_ingress.acl_wbb_ingress_trap") action acl_wbb_ingress_acl_wbb_ingress_trap_0() {
        acl_wbb_ingress_acl_wbb_ingress_meter.read(local_metadata._color32);
        clone(CloneType.I2E, 32w255);
        mark_to_drop(standard_metadata);
        acl_wbb_ingress_acl_wbb_ingress_counter.count();
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
  ") @name("ingress.acl_wbb_ingress.acl_wbb_ingress_table") table acl_wbb_ingress_acl_wbb_ingress_table {
        key = {
            headers.ipv4.isValid()            : optional @name("is_ipv4") @id(1) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()            : optional @name("is_ipv6") @id(2) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type       : ternary @name("ether_type") @id(3) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            acl_wbb_ingress_ttl               : ternary @name("ttl") @id(4) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            headers.ipv4.header_checksum[11:0]: ternary @name("outer_vlan_id") @id(5) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID);
        }
        actions = {
            @proto_id(1) acl_wbb_ingress_acl_wbb_ingress_copy_0();
            @proto_id(2) acl_wbb_ingress_acl_wbb_ingress_trap_0();
            @defaultonly NoAction_1();
        }
        const default_action = NoAction_1();
        meters = acl_wbb_ingress_acl_wbb_ingress_meter;
        counters = acl_wbb_ingress_acl_wbb_ingress_counter;
        size = 8;
    }
    @hidden action pins_wbb325() {
        acl_wbb_ingress_ttl = headers.ipv4.ttl;
    }
    @hidden action pins_wbb327() {
        acl_wbb_ingress_ttl = headers.ipv6.hop_limit;
    }
    @hidden action pins_wbb262() {
        acl_wbb_ingress_ttl = 8w0;
    }
    @hidden table tbl_pins_wbb262 {
        actions = {
            pins_wbb262();
        }
        const default_action = pins_wbb262();
    }
    @hidden table tbl_pins_wbb325 {
        actions = {
            pins_wbb325();
        }
        const default_action = pins_wbb325();
    }
    @hidden table tbl_pins_wbb327 {
        actions = {
            pins_wbb327();
        }
        const default_action = pins_wbb327();
    }
    apply {
        tbl_pins_wbb262.apply();
        if (headers.ipv4.isValid()) {
            tbl_pins_wbb325.apply();
        } else if (headers.ipv6.isValid()) {
            tbl_pins_wbb327.apply();
        }
        acl_wbb_ingress_acl_wbb_ingress_table.apply();
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
