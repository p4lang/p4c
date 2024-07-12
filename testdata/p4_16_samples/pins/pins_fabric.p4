#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
typedef bit<16> ether_type_t;
const vlan_id_t INTERNAL_VLAN_ID = 0xfff;
const vlan_id_t NO_VLAN_ID = 0x0;
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

@p4runtime_translation("" , string) type bit<10> nexthop_id_t;
@p4runtime_translation("" , string) type bit<10> tunnel_id_t;
@p4runtime_translation("" , string) type bit<12> wcmp_group_id_t;
@p4runtime_translation("" , string) @p4runtime_translation_mappings({ { "" , 0 } , }) type bit<10> vrf_id_t;
const vrf_id_t kDefaultVrf = 0;
@p4runtime_translation("" , string) type bit<10> router_interface_id_t;
@p4runtime_translation("" , string) type bit<9> port_id_t;
@p4runtime_translation("" , string) type bit<10> mirror_session_id_t;
@p4runtime_translation("" , string) type bit<8> qos_queue_t;
typedef bit<6> route_metadata_t;
typedef bit<8> acl_metadata_t;
typedef bit<16> multicast_group_id_t;
typedef bit<16> replica_instance_t;
enum bit<2> MeterColor_t {
    GREEN = 0,
    YELLOW = 1,
    RED = 2
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

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        local_metadata.enable_vlan_checks = false;
        local_metadata.vlan_id = 0;
        local_metadata.admit_to_l3 = false;
        local_metadata.vrf_id = kDefaultVrf;
        local_metadata.enable_decrement_ttl = false;
        local_metadata.enable_src_mac_rewrite = false;
        local_metadata.enable_dst_mac_rewrite = false;
        local_metadata.enable_vlan_rewrite = false;
        local_metadata.packet_rewrites.src_mac = 0;
        local_metadata.packet_rewrites.dst_mac = 0;
        local_metadata.l4_src_port = 0;
        local_metadata.l4_dst_port = 0;
        local_metadata.wcmp_selector_input = 0;
        local_metadata.apply_tunnel_decap_at_end_of_pre_ingress = false;
        local_metadata.apply_tunnel_encap_at_egress = false;
        local_metadata.tunnel_encap_src_ipv6 = 0;
        local_metadata.tunnel_encap_dst_ipv6 = 0;
        local_metadata.marked_to_copy = false;
        local_metadata.marked_to_mirror = false;
        local_metadata.mirror_session_id = 0;
        local_metadata.mirror_egress_port = 0;
        local_metadata.color = MeterColor_t.GREEN;
        local_metadata.route_metadata = 0;
        local_metadata.bypass_ingress = false;
        local_metadata.wcmp_group_id_valid = false;
        local_metadata.wcmp_group_id_value = 0;
        local_metadata.nexthop_id_valid = false;
        local_metadata.nexthop_id_value = 0;
        local_metadata.ipmc_table_hit = false;
        local_metadata.acl_drop = false;
        if (standard_metadata.instance_type == 4) {
            local_metadata.ingress_port = (port_id_t)local_metadata.loopback_port;
        } else {
            local_metadata.ingress_port = (port_id_t)standard_metadata.ingress_port;
        }
        transition select(standard_metadata.ingress_port) {
            510: parse_packet_out_header;
            default: parse_ethernet;
        }
    }
    state parse_packet_out_header {
        packet.extract(headers.packet_out_header);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            0x800: parse_ipv4;
            0x86dd: parse_ipv6;
            0x806: parse_arp;
            0x8100: parse_8021q_vlan;
            default: accept;
        }
    }
    state parse_8021q_vlan {
        packet.extract(headers.vlan);
        transition select(headers.vlan.ether_type) {
            0x800: parse_ipv4;
            0x86dd: parse_ipv6;
            0x806: parse_arp;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(headers.ipv4);
        transition select(headers.ipv4.protocol) {
            0x4: parse_ipv4_in_ip;
            0x29: parse_ipv6_in_ip;
            0x1: parse_icmp;
            0x6: parse_tcp;
            0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv4_in_ip {
        packet.extract(headers.inner_ipv4);
        transition select(headers.inner_ipv4.protocol) {
            0x1: parse_icmp;
            0x6: parse_tcp;
            0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract(headers.ipv6);
        transition select(headers.ipv6.next_header) {
            0x4: parse_ipv4_in_ip;
            0x29: parse_ipv6_in_ip;
            0x3a: parse_icmp;
            0x6: parse_tcp;
            0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6_in_ip {
        packet.extract(headers.inner_ipv6);
        transition select(headers.inner_ipv6.next_header) {
            0x3a: parse_icmp;
            0x6: parse_tcp;
            0x11: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract(headers.tcp);
        local_metadata.l4_src_port = headers.tcp.src_port;
        local_metadata.l4_dst_port = headers.tcp.dst_port;
        transition accept;
    }
    state parse_udp {
        packet.extract(headers.udp);
        local_metadata.l4_src_port = headers.udp.src_port;
        local_metadata.l4_dst_port = headers.udp.dst_port;
        transition accept;
    }
    state parse_icmp {
        packet.extract(headers.icmp);
        transition accept;
    }
    state parse_arp {
        packet.extract(headers.arp);
        transition accept;
    }
}

control packet_deparser(packet_out packet, in headers_t headers) {
    apply {
        packet.emit(headers.packet_out_header);
        packet.emit(headers.mirror_encap_ethernet);
        packet.emit(headers.mirror_encap_vlan);
        packet.emit(headers.mirror_encap_ipv6);
        packet.emit(headers.mirror_encap_udp);
        packet.emit(headers.ipfix);
        packet.emit(headers.psamp_extended);
        packet.emit(headers.ethernet);
        packet.emit(headers.vlan);
        packet.emit(headers.tunnel_encap_ipv6);
        packet.emit(headers.tunnel_encap_gre);
        packet.emit(headers.ipv4);
        packet.emit(headers.ipv6);
        packet.emit(headers.inner_ipv4);
        packet.emit(headers.inner_ipv6);
        packet.emit(headers.arp);
        packet.emit(headers.icmp);
        packet.emit(headers.tcp);
        packet.emit(headers.udp);
    }
}

control packet_in_encap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control packet_out_decap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (headers.packet_out_header.isValid() && headers.packet_out_header.submit_to_ingress == 0) {
            standard_metadata.egress_spec = (bit<9>)headers.packet_out_header.egress_port;
            local_metadata.bypass_ingress = true;
        }
        headers.packet_out_header.setInvalid();
    }
}

@id(0x01000005) action set_nexthop_id(inout local_metadata_t local_metadata, @id(1) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id) {
    local_metadata.nexthop_id_valid = true;
    local_metadata.nexthop_id_value = nexthop_id;
}
control routing_lookup(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x01798B9E) action no_action() {
    }
    @entry_restriction("
    // The VRF ID 0 (or '' in P4Runtime) encodes the default VRF, which cannot
    // be read or written via this table, but is always present implicitly.
    // TODO: This constraint should read `vrf_id != ''` (since
    // constraints are a control plane (P4Runtime) concept), but
    // p4-constraints does not currently support strings.
    vrf_id != 0;
  ") @p4runtime_role("sdn_controller") @id(0x0200004A) table vrf_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id");
        }
        actions = {
            @proto_id(1) no_action;
        }
        const default_action = no_action;
        size = 64;
    }
    @id(0x01000006) action drop() {
        mark_to_drop(standard_metadata);
    }
    @id(0x01000004) action set_wcmp_group_id(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id) {
        local_metadata.wcmp_group_id_valid = true;
        local_metadata.wcmp_group_id_value = wcmp_group_id;
    }
    @id(0x01000011) action set_wcmp_group_id_and_metadata(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id, route_metadata_t route_metadata) {
        set_wcmp_group_id(wcmp_group_id);
        local_metadata.route_metadata = route_metadata;
    }
    @id(0x01000015) action set_metadata_and_drop(@id(1) route_metadata_t route_metadata) {
        local_metadata.route_metadata = route_metadata;
        mark_to_drop(standard_metadata);
    }
    @id(0x01000010) action set_nexthop_id_and_metadata(@id(1) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id, route_metadata_t route_metadata) {
        local_metadata.nexthop_id_valid = true;
        local_metadata.nexthop_id_value = nexthop_id;
        local_metadata.route_metadata = route_metadata;
    }
    @id(0x01000018) @action_restriction("
    // Disallow 0 since it encodes 'no multicast' in V1Model.
    multicast_group_id != 0;
  ") action set_multicast_group_id(@id(1) @refers_to(builtin : : multicast_group_table , multicast_group_id) multicast_group_id_t multicast_group_id) {
        standard_metadata.mcast_grp = multicast_group_id;
    }
    @p4runtime_role("sdn_controller") @id(0x02000044) table ipv4_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv4.dst_addr: lpm @id(2) @name("ipv4_dst") @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) drop;
            @proto_id(2) set_nexthop_id(local_metadata);
            @proto_id(3) set_wcmp_group_id;
            @proto_id(5) set_nexthop_id_and_metadata;
            @proto_id(6) set_wcmp_group_id_and_metadata;
            @proto_id(7) set_metadata_and_drop;
        }
        const default_action = drop;
        size = 131072;
    }
    @p4runtime_role("sdn_controller") @id(0x02000045) table ipv6_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv6.dst_addr: lpm @id(2) @name("ipv6_dst") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) drop;
            @proto_id(2) set_nexthop_id(local_metadata);
            @proto_id(3) set_wcmp_group_id;
            @proto_id(5) set_nexthop_id_and_metadata;
            @proto_id(6) set_wcmp_group_id_and_metadata;
            @proto_id(7) set_metadata_and_drop;
        }
        const default_action = drop;
        size = 17000;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004E) table ipv4_multicast_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv4.dst_addr: exact @id(2) @name("ipv4_dst") @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) set_multicast_group_id;
        }
        size = 1600;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004F) table ipv6_multicast_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv6.dst_addr: exact @id(2) @name("ipv6_dst") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) set_multicast_group_id;
        }
        size = 1600;
    }
    apply {
        mark_to_drop(standard_metadata);
        vrf_table.apply();
        if (local_metadata.admit_to_l3) {
            if (headers.ipv4.isValid()) {
                ipv4_table.apply();
                ipv4_multicast_table.apply();
                local_metadata.ipmc_table_hit = standard_metadata.mcast_grp != 0;
            } else if (headers.ipv6.isValid()) {
                ipv6_table.apply();
                ipv6_multicast_table.apply();
                local_metadata.ipmc_table_hit = standard_metadata.mcast_grp != 0;
            }
        }
    }
}

control routing_resolution(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bool tunnel_id_valid = false;
    tunnel_id_t tunnel_id_value;
    bool router_interface_id_valid = false;
    router_interface_id_t router_interface_id_value;
    bool neighbor_id_valid = false;
    ipv6_addr_t neighbor_id_value;
    @id(0x01000001) action set_dst_mac(@id(1) @format(MAC_ADDRESS) ethernet_addr_t dst_mac) {
        local_metadata.packet_rewrites.dst_mac = dst_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x02000040) table neighbor_table {
        key = {
            router_interface_id_value: exact @id(1) @name("router_interface_id") @refers_to(router_interface_table , router_interface_id);
            neighbor_id_value        : exact @id(2) @format(IPV6_ADDRESS) @name("neighbor_id");
        }
        actions = {
            @proto_id(1) set_dst_mac;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 1024;
    }
    @id(0x0100001B) @unsupported @action_restriction("
    // Disallow reserved VLAN IDs with implementation-defined semantics.
    vlan_id != 0 && vlan_id != 4095") action set_port_and_src_mac_and_vlan_id(@id(1) port_id_t port, @id(2) @format(MAC_ADDRESS) ethernet_addr_t src_mac, @id(3) vlan_id_t vlan_id) {
        standard_metadata.egress_spec = (bit<9>)port;
        local_metadata.packet_rewrites.src_mac = src_mac;
        local_metadata.packet_rewrites.vlan_id = vlan_id;
    }
    @id(0x01000002) action set_port_and_src_mac(@id(1) port_id_t port, @id(2) @format(MAC_ADDRESS) ethernet_addr_t src_mac) {
        set_port_and_src_mac_and_vlan_id(port, src_mac, INTERNAL_VLAN_ID);
    }
    @p4runtime_role("sdn_controller") @id(0x02000041) table router_interface_table {
        key = {
            router_interface_id_value: exact @id(1) @name("router_interface_id");
        }
        actions = {
            @proto_id(1) set_port_and_src_mac;
            @proto_id(2) set_port_and_src_mac_and_vlan_id;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 256;
    }
    @id(0x01000017) action set_ip_nexthop_and_disable_rewrites(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id, @id(3) bit<1> disable_decrement_ttl, @id(4) bit<1> disable_src_mac_rewrite, @id(5) bit<1> disable_dst_mac_rewrite, @id(6) bit<1> disable_vlan_rewrite) {
        router_interface_id_valid = true;
        router_interface_id_value = router_interface_id;
        neighbor_id_valid = true;
        neighbor_id_value = neighbor_id;
        local_metadata.enable_decrement_ttl = !(bool)disable_decrement_ttl;
        local_metadata.enable_src_mac_rewrite = !(bool)disable_src_mac_rewrite;
        local_metadata.enable_dst_mac_rewrite = !(bool)disable_dst_mac_rewrite;
        local_metadata.enable_vlan_rewrite = !(bool)disable_vlan_rewrite;
    }
    @id(0x01000014) action set_ip_nexthop(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        set_ip_nexthop_and_disable_rewrites(router_interface_id, neighbor_id, 0x0, 0x0, 0x0, 0x0);
    }
    @id(0x01000003) @deprecated("Use set_ip_nexthop instead.") action set_nexthop(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        set_ip_nexthop(router_interface_id, neighbor_id);
    }
    @id(0x01000012) action set_p2p_tunnel_encap_nexthop(@id(1) @refers_to(tunnel_table , tunnel_id) tunnel_id_t tunnel_id) {
        tunnel_id_valid = true;
        tunnel_id_value = tunnel_id;
    }
    @p4runtime_role("sdn_controller") @id(0x02000042) table nexthop_table {
        key = {
            local_metadata.nexthop_id_value: exact @id(1) @name("nexthop_id");
        }
        actions = {
            @proto_id(1) set_nexthop;
            @proto_id(2) set_p2p_tunnel_encap_nexthop;
            @proto_id(3) set_ip_nexthop;
            @proto_id(4) set_ip_nexthop_and_disable_rewrites;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 1024;
    }
    @id(0x01000013) action mark_for_p2p_tunnel_encap(@id(1) @format(IPV6_ADDRESS) ipv6_addr_t encap_src_ip, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t encap_dst_ip, @id(3) @refers_to(neighbor_table , router_interface_id) @refers_to(router_interface_table , router_interface_id) router_interface_id_t router_interface_id) {
        local_metadata.tunnel_encap_src_ipv6 = encap_src_ip;
        local_metadata.tunnel_encap_dst_ipv6 = encap_dst_ip;
        local_metadata.apply_tunnel_encap_at_egress = true;
        set_ip_nexthop(router_interface_id, encap_dst_ip);
    }
    @p4runtime_role("sdn_controller") @id(0x02000050) table tunnel_table {
        key = {
            tunnel_id_value: exact @id(1) @name("tunnel_id");
        }
        actions = {
            @proto_id(1) mark_for_p2p_tunnel_encap;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 2048;
    }
    @max_group_size(512) @id(0x11DC4EC8) action_selector(HashAlgorithm.identity, 49152, 8) wcmp_group_selector;
    @p4runtime_role("sdn_controller") @id(0x02000043) @oneshot table wcmp_group_table {
        key = {
            local_metadata.wcmp_group_id_value: exact @id(1) @name("wcmp_group_id");
            local_metadata.wcmp_selector_input: selector;
        }
        actions = {
            @proto_id(1) set_nexthop_id(local_metadata);
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        implementation = wcmp_group_selector;
        size = 3968;
    }
    apply {
        if (local_metadata.admit_to_l3) {
            if (local_metadata.wcmp_group_id_valid) {
                wcmp_group_table.apply();
            }
            if (local_metadata.nexthop_id_valid) {
                nexthop_table.apply();
                if (tunnel_id_valid) {
                    tunnel_table.apply();
                }
                if (router_interface_id_valid && neighbor_id_valid) {
                    router_interface_table.apply();
                    neighbor_table.apply();
                }
            }
        }
        local_metadata.packet_in_target_egress_port = standard_metadata.egress_spec;
        local_metadata.packet_in_ingress_port = standard_metadata.ingress_port;
        if (local_metadata.acl_drop) {
            mark_to_drop(standard_metadata);
        }
    }
}

control verify_ipv4_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        verify_checksum(headers.ipv4.isValid(), { headers.ipv4.version, headers.ipv4.ihl, headers.ipv4.dscp, headers.ipv4.ecn, headers.ipv4.total_len, headers.ipv4.identification, headers.ipv4.reserved, headers.ipv4.do_not_fragment, headers.ipv4.more_fragments, headers.ipv4.frag_offset, headers.ipv4.ttl, headers.ipv4.protocol, headers.ipv4.src_addr, headers.ipv4.dst_addr }, headers.ipv4.header_checksum, HashAlgorithm.csum16);
    }
}

control compute_ipv4_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        update_checksum(headers.ipv4.isValid(), { headers.ipv4.version, headers.ipv4.ihl, headers.ipv4.dscp, headers.ipv4.ecn, headers.ipv4.total_len, headers.ipv4.identification, headers.ipv4.reserved, headers.ipv4.do_not_fragment, headers.ipv4.more_fragments, headers.ipv4.frag_offset, headers.ipv4.ttl, headers.ipv4.protocol, headers.ipv4.src_addr, headers.ipv4.dst_addr }, headers.ipv4.header_checksum, HashAlgorithm.csum16);
    }
}

control ingress_cloning(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x0100001C) action ingress_clone(@id(1) bit<32> clone_session) {
        clone_preserving_field_list(CloneType.I2E, clone_session, PreservedFieldList.MIRROR_AND_PACKET_IN_COPY);
    }
    @unsupported @p4runtime_role("packet_replication_engine_manager") @id(0x02000051) @entry_restriction("
    // mirror_egress_port is present iff marked_to_mirror is true.
    // Exact match indicating presence of mirror_egress_port.
    marked_to_mirror == 1 -> mirror_egress_port::mask == -1;
    // Wildcard match indicating abscence of mirror_egress_port.
    marked_to_mirror == 0 -> mirror_egress_port::mask == 0;
  ") table ingress_clone_table {
        key = {
            local_metadata.marked_to_copy    : exact @id(1) @name("marked_to_copy");
            local_metadata.marked_to_mirror  : exact @id(2) @name("marked_to_mirror");
            local_metadata.mirror_egress_port: optional @id(3) @name("mirror_egress_port");
        }
        actions = {
            @proto_id(1) ingress_clone;
        }
    }
    apply {
        ingress_clone_table.apply();
    }
}

control mirror_session_lookup(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x01000007) action mirror_as_ipv4_erspan(@id(1) port_id_t port, @id(2) @format(IPV4_ADDRESS) ipv4_addr_t src_ip, @id(3) @format(IPV4_ADDRESS) ipv4_addr_t dst_ip, @id(4) @format(MAC_ADDRESS) ethernet_addr_t src_mac, @id(5) @format(MAC_ADDRESS) ethernet_addr_t dst_mac, @id(6) bit<8> ttl, @id(7) bit<8> tos) {
    }
    @id(0x0100001D) @unsupported action mirror_with_vlan_tag_and_ipfix_encapsulation(@id(1) port_id_t monitor_port, @id(2) port_id_t monitor_failover_port, @id(3) @format(MAC_ADDRESS) ethernet_addr_t mirror_encap_src_mac, @id(4) @format(MAC_ADDRESS) ethernet_addr_t mirror_encap_dst_mac, @id(6) vlan_id_t mirror_encap_vlan_id, @id(7) @format(IPV6_ADDRESS) ipv6_addr_t mirror_encap_dst_ip, @id(8) @format(IPV6_ADDRESS) ipv6_addr_t mirror_encap_src_ip, @id(9) bit<16> mirror_encap_udp_src_port, @id(10) bit<16> mirror_encap_udp_dst_port) {
        local_metadata.mirror_egress_port = monitor_port;
        local_metadata.mirror_encap_src_mac = mirror_encap_src_mac;
        local_metadata.mirror_encap_dst_mac = mirror_encap_dst_mac;
        local_metadata.mirror_encap_vlan_id = mirror_encap_vlan_id;
        local_metadata.mirror_encap_src_ip = mirror_encap_src_ip;
        local_metadata.mirror_encap_dst_ip = mirror_encap_dst_ip;
        local_metadata.mirror_encap_udp_src_port = mirror_encap_udp_src_port;
        local_metadata.mirror_encap_udp_dst_port = mirror_encap_udp_dst_port;
    }
    @p4runtime_role("sdn_controller") @id(0x02000046) table mirror_session_table {
        key = {
            local_metadata.mirror_session_id: exact @id(1) @name("mirror_session_id");
        }
        actions = {
            @proto_id(1) mirror_as_ipv4_erspan;
            @proto_id(2) mirror_with_vlan_tag_and_ipfix_encapsulation;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 4;
    }
    apply {
        if (local_metadata.marked_to_mirror) {
            mirror_session_table.apply();
        }
    }
}

control mirroring_encap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.instance_type == 1 && standard_metadata.egress_rid == 2) {
            headers.mirror_encap_ethernet.setValid();
            headers.mirror_encap_ethernet.src_addr = local_metadata.mirror_encap_src_mac;
            headers.mirror_encap_ethernet.dst_addr = local_metadata.mirror_encap_dst_mac;
            headers.mirror_encap_ethernet.ether_type = 0x8100;
            headers.mirror_encap_vlan.setValid();
            headers.mirror_encap_vlan.ether_type = 0x86dd;
            headers.mirror_encap_vlan.vlan_id = local_metadata.mirror_encap_vlan_id;
            headers.mirror_encap_ipv6.setValid();
            headers.mirror_encap_ipv6.version = 4w6;
            headers.mirror_encap_ipv6.dscp = 0;
            headers.mirror_encap_ipv6.ecn = 0;
            headers.mirror_encap_ipv6.hop_limit = 16;
            headers.mirror_encap_ipv6.flow_label = 0;
            headers.mirror_encap_ipv6.payload_length = (bit<16>)standard_metadata.packet_length + 8 + 16 + 28;
            headers.mirror_encap_ipv6.next_header = 0x11;
            headers.mirror_encap_ipv6.src_addr = local_metadata.mirror_encap_src_ip;
            headers.mirror_encap_ipv6.dst_addr = local_metadata.mirror_encap_dst_ip;
            headers.mirror_encap_udp.setValid();
            headers.mirror_encap_udp.src_port = local_metadata.mirror_encap_udp_src_port;
            headers.mirror_encap_udp.dst_port = local_metadata.mirror_encap_udp_dst_port;
            headers.mirror_encap_udp.hdr_length = headers.mirror_encap_ipv6.payload_length;
            headers.mirror_encap_udp.checksum = 0;
            headers.ipfix.setValid();
            headers.psamp_extended.setValid();
        }
    }
}

control l3_admit(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @id(0x01000008) action admit_to_l3() {
        local_metadata.admit_to_l3 = true;
    }
    @p4runtime_role("sdn_controller") @id(0x02000047) table l3_admit_table {
        key = {
            headers.ethernet.dst_addr  : ternary @name("dst_mac") @id(1) @format(MAC_ADDRESS);
            local_metadata.ingress_port: optional @name("in_port") @id(2);
        }
        actions = {
            @proto_id(1) admit_to_l3;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 64;
    }
    apply {
        if (local_metadata.enable_vlan_checks && !(local_metadata.vlan_id == NO_VLAN_ID || local_metadata.vlan_id == INTERNAL_VLAN_ID)) {
            local_metadata.admit_to_l3 = false;
        } else {
            l3_admit_table.apply();
        }
    }
}

control gre_tunnel_encap(inout headers_t headers, in local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.apply_tunnel_encap_at_egress) {
            headers.tunnel_encap_gre.setValid();
            headers.tunnel_encap_gre.checksum_present = 0;
            headers.tunnel_encap_gre.routing_present = 0;
            headers.tunnel_encap_gre.key_present = 0;
            headers.tunnel_encap_gre.sequence_present = 0;
            headers.tunnel_encap_gre.strict_source_route = 0;
            headers.tunnel_encap_gre.recursion_control = 0;
            headers.tunnel_encap_gre.flags = 0;
            headers.tunnel_encap_gre.version = 0;
            headers.tunnel_encap_gre.protocol = headers.ethernet.ether_type;
            headers.ethernet.ether_type = 0x86dd;
            headers.tunnel_encap_ipv6.setValid();
            headers.tunnel_encap_ipv6.version = 4w6;
            headers.tunnel_encap_ipv6.src_addr = local_metadata.tunnel_encap_src_ipv6;
            headers.tunnel_encap_ipv6.dst_addr = local_metadata.tunnel_encap_dst_ipv6;
            headers.tunnel_encap_ipv6.payload_length = (bit<16>)standard_metadata.packet_length + 4 - 14;
            headers.tunnel_encap_ipv6.next_header = 0x2f;
            if (headers.ipv4.isValid()) {
                headers.tunnel_encap_ipv6.dscp = headers.ipv4.dscp;
                headers.tunnel_encap_ipv6.ecn = headers.ipv4.ecn;
                headers.tunnel_encap_ipv6.hop_limit = headers.ipv4.ttl;
            } else if (headers.ipv6.isValid()) {
                headers.tunnel_encap_ipv6.dscp = headers.ipv6.dscp;
                headers.tunnel_encap_ipv6.ecn = headers.ipv6.ecn;
                headers.tunnel_encap_ipv6.hop_limit = headers.ipv6.hop_limit;
            }
        }
    }
}

control vlan_untag(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x0100001A) action disable_vlan_checks() {
        local_metadata.enable_vlan_checks = false;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004D) @entry_restriction("
    // Force the dummy_match to be wildcard.
    dummy_match::mask == 0;
  ") table disable_vlan_checks_table {
        key = {
            1w1: ternary @id(1) @name("dummy_match");
        }
        actions = {
            @proto_id(1) disable_vlan_checks;
        }
        size = 1;
    }
    apply {
        if (headers.vlan.isValid()) {
            local_metadata.vlan_id = headers.vlan.vlan_id;
            headers.ethernet.ether_type = headers.vlan.ether_type;
            headers.vlan.setInvalid();
        } else {
            local_metadata.vlan_id = INTERNAL_VLAN_ID;
        }
        local_metadata.enable_vlan_checks = true;
        disable_vlan_checks_table.apply();
    }
}

control ingress_vlan_checks(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.enable_vlan_checks && !(local_metadata.vlan_id == NO_VLAN_ID || local_metadata.vlan_id == INTERNAL_VLAN_ID)) {
            mark_to_drop(standard_metadata);
        }
    }
}

control egress_vlan_checks(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.enable_vlan_checks) {
            if (standard_metadata.instance_type == 1 && standard_metadata.egress_rid == 2 && !(local_metadata.mirror_encap_vlan_id == NO_VLAN_ID || local_metadata.mirror_encap_vlan_id == INTERNAL_VLAN_ID)) {
                mark_to_drop(standard_metadata);
            } else if (!(standard_metadata.instance_type == 1 && standard_metadata.egress_rid == 1) && !(local_metadata.vlan_id == NO_VLAN_ID || local_metadata.vlan_id == INTERNAL_VLAN_ID)) {
                mark_to_drop(standard_metadata);
            }
        }
    }
}

control vlan_tag(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (!(local_metadata.vlan_id == NO_VLAN_ID || local_metadata.vlan_id == INTERNAL_VLAN_ID) && !(standard_metadata.instance_type == 1 && standard_metadata.egress_rid == 2)) {
            headers.vlan.setValid();
            headers.vlan.priority_code_point = 0;
            headers.vlan.drop_eligible_indicator = 0;
            headers.vlan.vlan_id = local_metadata.vlan_id;
            headers.vlan.ether_type = headers.ethernet.ether_type;
            headers.ethernet.ether_type = 0x8100;
        }
    }
}

const ipv6_addr_t IPV6_MULTICAST_MASK = 0xff000000000000000000000000000000;
const ipv6_addr_t IPV6_MULTICAST_VALUE = 0xff000000000000000000000000000000;
const ipv6_addr_t IPV6_LOOPBACK_MASK = 0xffffffffffffffffffffffffffffffff;
const ipv6_addr_t IPV6_LOOPBACK_VALUE = 0x1;
const ipv4_addr_t IPV4_MULTICAST_MASK = 0xf0000000;
const ipv4_addr_t IPV4_MULTICAST_VALUE = 0xe0000000;
const ipv4_addr_t IPV4_BROADCAST_VALUE = 0xffffffff;
const ipv4_addr_t IPV4_LOOPBACK_MASK = 0xff000000;
const ipv4_addr_t IPV4_LOOPBACK_VALUE = 0x7f000000;
const ethernet_addr_t MAC_MULTICAST_MASK = 0x10000000000;
const ethernet_addr_t MAC_MULTICAST_VALUE = 0x10000000000;
control drop_martians(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (headers.ipv6.isValid() && (headers.ipv6.src_addr & IPV6_MULTICAST_MASK == IPV6_MULTICAST_VALUE || headers.ipv6.dst_addr & IPV6_MULTICAST_MASK == IPV6_MULTICAST_VALUE || headers.ipv6.src_addr & IPV6_LOOPBACK_MASK == IPV6_LOOPBACK_VALUE || headers.ipv6.dst_addr & IPV6_LOOPBACK_MASK == IPV6_LOOPBACK_VALUE) || headers.ipv4.isValid() && (headers.ipv4.src_addr & IPV4_MULTICAST_MASK == IPV4_MULTICAST_VALUE || headers.ipv4.src_addr == IPV4_BROADCAST_VALUE || (headers.ipv4.dst_addr & IPV4_MULTICAST_MASK == IPV4_MULTICAST_VALUE || headers.ipv4.dst_addr == IPV4_BROADCAST_VALUE) || headers.ipv4.src_addr & IPV4_LOOPBACK_MASK == IPV4_LOOPBACK_VALUE || headers.ipv4.dst_addr & IPV4_LOOPBACK_MASK == IPV4_LOOPBACK_VALUE) || headers.ethernet.isValid() && headers.ethernet.dst_addr & MAC_MULTICAST_MASK == MAC_MULTICAST_VALUE) {
            mark_to_drop(standard_metadata);
        }
    }
}

control multicast_rewrites(inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    port_id_t multicast_replica_port = (port_id_t)standard_metadata.egress_port;
    replica_instance_t multicast_replica_instance = standard_metadata.egress_rid;
    @id(0x01000019) action set_multicast_src_mac(@id(1) @format(MAC_ADDRESS) ethernet_addr_t src_mac) {
        local_metadata.enable_src_mac_rewrite = true;
        local_metadata.packet_rewrites.src_mac = src_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004C) table multicast_router_interface_table {
        key = {
            multicast_replica_port    : exact @referenced_by(builtin : : multicast_group_table , replica . port) @id(1);
            multicast_replica_instance: exact @referenced_by(builtin : : multicast_group_table , replica . instance) @id(2);
        }
        actions = {
            @proto_id(1) set_multicast_src_mac;
        }
        size = 110;
    }
    apply {
        multicast_router_interface_table.apply();
    }
}

control packet_rewrites(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.instance_type == 5) {
            local_metadata.enable_decrement_ttl = true;
            multicast_rewrites.apply(local_metadata, standard_metadata);
        }
        if (local_metadata.enable_src_mac_rewrite) {
            headers.ethernet.src_addr = local_metadata.packet_rewrites.src_mac;
        }
        if (local_metadata.enable_dst_mac_rewrite) {
            headers.ethernet.dst_addr = local_metadata.packet_rewrites.dst_mac;
        }
        if (local_metadata.enable_vlan_rewrite) {
            local_metadata.vlan_id = local_metadata.packet_rewrites.vlan_id;
        }
        if (headers.ipv4.isValid()) {
            if (headers.ipv4.ttl > 0 && local_metadata.enable_decrement_ttl) {
                headers.ipv4.ttl = headers.ipv4.ttl - 1;
            }
            if (headers.ipv4.ttl == 0) {
                mark_to_drop(standard_metadata);
            }
        }
        if (headers.ipv6.isValid()) {
            if (headers.ipv6.hop_limit > 0 && local_metadata.enable_decrement_ttl) {
                headers.ipv6.hop_limit = headers.ipv6.hop_limit - 1;
            }
            if (headers.ipv6.hop_limit == 0) {
                mark_to_drop(standard_metadata);
            }
        }
    }
}

control tunnel_termination_lookup(in headers_t headers, inout local_metadata_t local_metadata) {
    @id(0x01000016) action mark_for_tunnel_decap_and_set_vrf(@refers_to(vrf_table , vrf_id) vrf_id_t vrf_id) {
        local_metadata.apply_tunnel_decap_at_end_of_pre_ingress = true;
        local_metadata.vrf_id = vrf_id;
    }
    @unsupported @p4runtime_role("sdn_controller") @id(0x0200004B) table ipv6_tunnel_termination_table {
        key = {
            headers.ipv6.dst_addr: ternary @id(1) @name("dst_ipv6") @format(IPV6_ADDRESS);
            headers.ipv6.src_addr: ternary @id(2) @name("src_ipv6") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) mark_for_tunnel_decap_and_set_vrf;
        }
        size = 126;
    }
    apply {
        if (headers.ipv6.isValid()) {
            if (headers.ipv6.next_header == 0x4 || headers.ipv6.next_header == 0x29) {
                ipv6_tunnel_termination_table.apply();
            }
        }
    }
}

control tunnel_termination_decap(inout headers_t headers, in local_metadata_t local_metadata) {
    apply {
        if (local_metadata.apply_tunnel_decap_at_end_of_pre_ingress) {
            assert(headers.ipv6.isValid());
            assert(headers.inner_ipv4.isValid() && !headers.inner_ipv6.isValid() || !headers.inner_ipv4.isValid() && headers.inner_ipv6.isValid());
            headers.ipv6.setInvalid();
            if (headers.inner_ipv4.isValid()) {
                headers.ethernet.ether_type = 0x800;
                headers.ipv4 = headers.inner_ipv4;
                headers.inner_ipv4.setInvalid();
            }
            if (headers.inner_ipv6.isValid()) {
                headers.ethernet.ether_type = 0x86dd;
                headers.ipv6 = headers.inner_ipv6;
                headers.inner_ipv6.setInvalid();
            }
        }
    }
}

@id(0x01000109) @sai_action(SAI_PACKET_ACTION_DROP) action acl_drop(inout local_metadata_t local_metadata) {
    local_metadata.acl_drop = true;
}
control acl_egress(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bit<6> dscp = 0;
    bit<8> ip_protocol = 0;
    @id(0x13000104) direct_counter(CounterType.packets_and_bytes) acl_egress_counter;
    @id(0x13000108) direct_counter(CounterType.packets_and_bytes) acl_egress_dhcp_to_host_counter;
    @id(0x0100010D) @sai_action(SAI_PACKET_ACTION_FORWARD) action acl_egress_forward() {
        acl_egress_counter.count();
    }
    @p4runtime_role("sdn_controller") @id(0x02000104) @sai_acl(EGRESS) @entry_restriction("

    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);

    // Only allow IP field matches for IP packets.
    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);




    // Only allow l4_dst_port matches for TCP/UDP packets.
    l4_dst_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);

    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_egress_table {
        key = {
            headers.ethernet.ether_type                     : ternary @id(1) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            ip_protocol                                     : ternary @id(2) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            local_metadata.l4_dst_port                      : ternary @id(3) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            (port_id_t)standard_metadata.egress_port        : optional @id(4) @name("out_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT);
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(5) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(6) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(7) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            dscp                                            : ternary @id(8) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
        }
        actions = {
            @proto_id(1) acl_drop(local_metadata);
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_egress_counter;
        size = 127;
    }
    @id(0x02000108) @sai_acl(EGRESS) @p4runtime_role("sdn_controller") @entry_restriction("
    // Only allow IP field matches for IP packets.
    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Only allow l4_dst_port matches for TCP/UDP packets.
    l4_dst_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_egress_dhcp_to_host_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            ip_protocol                                     : ternary @id(5) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            local_metadata.l4_dst_port                      : ternary @id(6) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            (port_id_t)standard_metadata.egress_port        : optional @id(7) @name("out_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT);
        }
        actions = {
            @proto_id(1) acl_drop(local_metadata);
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_egress_dhcp_to_host_counter;
        size = 127;
    }
    apply {
        if (headers.ipv4.isValid()) {
            dscp = headers.ipv4.dscp;
            ip_protocol = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            dscp = headers.ipv6.dscp;
            ip_protocol = headers.ipv6.next_header;
        } else {
            ip_protocol = 0;
        }
        acl_egress_table.apply();
        if (local_metadata.acl_drop) {
            mark_to_drop(standard_metadata);
        }
    }
}

control acl_ingress(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bit<8> ttl = 0;
    bit<6> dscp = 0;
    bit<2> ecn = 0;
    bit<8> ip_protocol = 0;
    bool cancel_copy = false;
    @id(0x15000100) @mode(single_rate_two_color) direct_meter<MeterColor_t>(MeterType.bytes) acl_ingress_meter;
    @id(0x15000102) @mode(single_rate_two_color) direct_meter<MeterColor_t>(MeterType.bytes) acl_ingress_qos_meter;
    @id(0x13000102) direct_counter(CounterType.packets_and_bytes) acl_ingress_counter;
    @id(0x13000107) direct_counter(CounterType.packets_and_bytes) acl_ingress_qos_counter;
    @id(0x13000109) direct_counter(CounterType.packets_and_bytes) acl_ingress_counting_counter;
    @id(0x1300010A) direct_counter(CounterType.packets_and_bytes) acl_ingress_security_counter;
    @id(0x01000101) @sai_action(SAI_PACKET_ACTION_COPY , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_RED) action acl_copy(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_ingress_counter.count();
        acl_ingress_meter.read(local_metadata.color);
        local_metadata.marked_to_copy = true;
    }
    @id(0x01000102) @sai_action(SAI_PACKET_ACTION_TRAP , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) action acl_trap(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_copy(qos_queue);
        local_metadata.acl_drop = true;
    }
    @id(0x01000103) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) action acl_forward() {
        acl_ingress_meter.read(local_metadata.color);
    }
    @id(0x01000105) @sai_action(SAI_PACKET_ACTION_FORWARD) action acl_count() {
        acl_ingress_counting_counter.count();
    }
    @id(0x01000104) @sai_action(SAI_PACKET_ACTION_FORWARD) action acl_mirror(@id(1) @refers_to(mirror_session_table , mirror_session_id) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS) mirror_session_id_t mirror_session_id) {
        acl_ingress_counter.count();
        local_metadata.marked_to_mirror = true;
        local_metadata.mirror_session_id = mirror_session_id;
    }
    @id(0x0100010C) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_COPY_CANCEL , SAI_PACKET_COLOR_RED) action set_qos_queue_and_cancel_copy_above_rate_limit(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t qos_queue) {
        acl_ingress_qos_meter.read(local_metadata.color);
    }
    @id(0x01000111) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DENY , SAI_PACKET_COLOR_RED) @unsupported action set_cpu_and_multicast_queues_and_deny_above_rate_limit(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue, @id(2) @sai_action_param(MULTICAST_QOS_QUEUE , SAI_PACKET_COLOR_GREEN) qos_queue_t green_multicast_queue, @id(3) @sai_action_param(MULTICAST_QOS_QUEUE , SAI_PACKET_COLOR_RED) qos_queue_t red_multicast_queue) {
        acl_ingress_qos_meter.read(local_metadata.color);
    }
    @id(0x0100010E) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DENY , SAI_PACKET_COLOR_RED) action set_cpu_queue_and_deny_above_rate_limit(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue) {
        acl_ingress_qos_meter.read(local_metadata.color);
    }
    @id(0x01000110) @sai_action(SAI_PACKET_ACTION_FORWARD) action set_cpu_queue(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue) {
    }
    @id(0x0100010F) @sai_action(SAI_PACKET_ACTION_DENY) action acl_deny() {
        cancel_copy = true;
        local_metadata.acl_drop = true;
    }
    @p4runtime_role("sdn_controller") @id(0x02000100) @sai_acl(INGRESS) @sai_acl_priority(5) @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;
    // Only allow IP field matches for IP packets.
    dst_ip::mask != 0 -> is_ipv4 == 1;

    src_ip::mask != 0 -> is_ipv4 == 1;

    dst_ipv6::mask != 0 -> is_ipv6 == 1;
    src_ipv6::mask != 0 -> is_ipv6 == 1;
    ttl::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);

    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ecn::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);

    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Only allow l4_dst_port and l4_src_port matches for TCP/UDP packets.

    l4_src_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);

    l4_dst_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);





    // Only allow icmp_type matches for ICMP packets
    icmp_type::mask != 0 -> ip_protocol == 1;

    icmpv6_type::mask != 0 -> ip_protocol == 58;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_ingress_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            headers.ethernet.dst_addr                       : ternary @id(5) @name("dst_mac") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_MAC) @format(MAC_ADDRESS);
            headers.ipv4.src_addr                           : ternary @id(6) @name("src_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_IP) @format(IPV4_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @id(7) @name("dst_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.src_addr[127:64]                   : ternary @id(8) @name("src_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @id(9) @name("dst_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            ttl                                             : ternary @id(10) @name("ttl") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            dscp                                            : ternary @id(11) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn                                             : ternary @id(12) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
            ip_protocol                                     : ternary @id(13) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(19) @name("icmp_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE);
            headers.icmp.type                               : ternary @id(14) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            local_metadata.l4_src_port                      : ternary @id(20) @name("l4_src_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT);
            local_metadata.l4_dst_port                      : ternary @id(15) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            local_metadata.ingress_port                     : optional @id(17) @name("in_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IN_PORT);
            local_metadata.route_metadata                   : optional @id(18) @name("route_metadata") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META);
        }
        actions = {
            @proto_id(1) acl_copy();
            @proto_id(2) acl_trap();
            @proto_id(3) acl_forward();
            @proto_id(4) acl_mirror();
            @proto_id(5) acl_drop(local_metadata);
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        meters = acl_ingress_meter;
        counters = acl_ingress_counter;
        size = 255;
    }
    @id(0x02000107) @sai_acl(INGRESS) @sai_acl_priority(10) @p4runtime_role("sdn_controller") @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;
    // Only allow IP field matches for IP packets.
    ttl::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Only allow l4_dst_port matches for TCP/UDP packets.
    l4_dst_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
    // Only allow icmp_type matches for ICMP packets
    icmpv6_type::mask != 0 -> ip_protocol == 58;

    // Only allow l4_dst_port matches for TCP/UDP packets.
    l4_src_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);
    // Only allow icmp_type matches for ICMP packets
    icmp_type::mask != 0 -> ip_protocol == 1;





  ") table acl_ingress_qos_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            ttl                                             : ternary @id(7) @name("ttl") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            ip_protocol                                     : ternary @id(8) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(9) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            local_metadata.l4_dst_port                      : ternary @id(10) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            local_metadata.l4_src_port                      : ternary @id(12) @name("l4_src_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT);
            headers.icmp.type                               : ternary @id(14) @name("icmp_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMP_TYPE);
            local_metadata.route_metadata                   : ternary @id(15) @name("route_metadata") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META);
        }
        actions = {
            @proto_id(1) set_qos_queue_and_cancel_copy_above_rate_limit();
            @proto_id(2) set_cpu_queue_and_deny_above_rate_limit();
            @proto_id(3) acl_forward();
            @proto_id(4) acl_drop(local_metadata);
            @proto_id(5) set_cpu_queue();
            @proto_id(6) set_cpu_and_multicast_queues_and_deny_above_rate_limit();
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        meters = acl_ingress_qos_meter;
        counters = acl_ingress_qos_counter;
        size = 511;
    }
    @p4runtime_role("sdn_controller") @id(0x02000109) @sai_acl_priority(7) @sai_acl(INGRESS) @entry_restriction("
    // Only allow IP field matches for IP packets.
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_ingress_counting_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            dscp                                            : ternary @id(11) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            local_metadata.route_metadata                   : ternary @id(18) @name("route_metadata") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META);
        }
        actions = {
            @proto_id(3) acl_count();
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_ingress_counting_counter;
        size = 255;
    }
    @id(0x01000112) action redirect_to_nexthop(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT) @sai_action_param_object_type(SAI_OBJECT_TYPE_NEXT_HOP) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id) {
        local_metadata.nexthop_id_valid = true;
        local_metadata.nexthop_id_value = nexthop_id;
        local_metadata.wcmp_group_id_valid = false;
        standard_metadata.mcast_grp = 0;
    }
    @id(0x01000113) @action_restriction("
    // Disallow 0 since it encodes 'no multicast' in V1Model.
    multicast_group_id != 0;
  ") action redirect_to_ipmc_group(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT) @sai_action_param_object_type(SAI_OBJECT_TYPE_IPMC_GROUP) @refers_to(builtin : : multicast_group_table , multicast_group_id) multicast_group_id_t multicast_group_id) {
        standard_metadata.mcast_grp = multicast_group_id;
        local_metadata.nexthop_id_valid = false;
        local_metadata.wcmp_group_id_valid = false;
    }
    @id(0x0200010B) @sai_acl(INGRESS) @sai_acl_priority(15) @p4runtime_role("sdn_controller") @entry_restriction("
    // Only allow IP field matches for IP packets.
    dst_ip::mask != 0 -> is_ipv4 == 1;
    dst_ipv6::mask != 0 -> is_ipv6 == 1;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_ingress_mirror_and_redirect_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(2) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(3) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(4) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ipv4.dst_addr                           : ternary @id(10) @name("dst_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @id(5) @name("dst_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            local_metadata.vrf_id                           : optional @id(8) @name("vrf_id") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_VRF_ID);
            local_metadata.ipmc_table_hit                   : optional @id(9) @name("ipmc_table_hit") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IPMC_NPU_META_DST_HIT);
        }
        actions = {
            @proto_id(4) acl_forward();
            @proto_id(1) acl_mirror();
            @proto_id(2) redirect_to_nexthop();
            @proto_id(3) redirect_to_ipmc_group();
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        size = 255;
    }
    @id(0x0200010A) @sai_acl(INGRESS) @sai_acl_priority(20) @p4runtime_role("sdn_controller") @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;






  // TODO: This comment is required for the preprocessor to not
  // spit out nonsense.







    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_ingress_security_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
        }
        actions = {
            @proto_id(1) acl_forward();
            @proto_id(2) acl_drop(local_metadata);
            @proto_id(3) acl_deny();
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_ingress_security_counter;
        size = 255;
    }
    apply {
        if (headers.ipv4.isValid()) {
            ttl = headers.ipv4.ttl;
            dscp = headers.ipv4.dscp;
            ecn = headers.ipv4.ecn;
            ip_protocol = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            ttl = headers.ipv6.hop_limit;
            dscp = headers.ipv6.dscp;
            ecn = headers.ipv6.ecn;
            ip_protocol = headers.ipv6.next_header;
        }
        acl_ingress_table.apply();
        acl_ingress_counting_table.apply();
        acl_ingress_qos_table.apply();
        if (cancel_copy) {
            local_metadata.marked_to_copy = false;
        }
    }
}

control acl_pre_ingress(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    bit<6> dscp = 0;
    bit<2> ecn = 0;
    bit<8> ip_protocol = 0;
    @id(0x13000101) direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_counter;
    @id(0x13000106) direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_vlan_counter;
    @id(0x13000105) direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_metadata_counter;
    @id(0x01000100) @sai_action(SAI_PACKET_ACTION_FORWARD) action set_vrf(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_VRF) @refers_to(vrf_table , vrf_id) @id(1) vrf_id_t vrf_id) {
        local_metadata.vrf_id = vrf_id;
        acl_pre_ingress_counter.count();
    }
    @id(0x0100010A) @sai_action(SAI_PACKET_ACTION_FORWARD) action set_outer_vlan_id(@id(1) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID) vlan_id_t vlan_id) {
        local_metadata.vlan_id = vlan_id;
        acl_pre_ingress_vlan_counter.count();
    }
    @id(0x0100010B) @sai_action(SAI_PACKET_ACTION_FORWARD) action set_acl_metadata(@id(1) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA) acl_metadata_t acl_metadata) {
        local_metadata.acl_metadata = acl_metadata;
        acl_pre_ingress_metadata_counter.count();
    }
    @p4runtime_role("sdn_controller") @id(0x02000101) @sai_acl(PRE_INGRESS) @sai_acl_priority(11) @entry_restriction("
    // Only allow IP field matches for IP packets.
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ecn::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    dst_ip::mask != 0 -> is_ipv4 == 1;
    dst_ipv6::mask != 0 -> is_ipv6 == 1;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);




    // Reserve high priorities for switch-internal use.
    // TODO: Remove once inband workaround is obsolete.
    ::priority < 0x7fffffff;
  ") table acl_pre_ingress_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.src_addr                       : ternary @id(4) @name("src_mac") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC) @format(MAC_ADDRESS);
            headers.ethernet.dst_addr                       : ternary @id(9) @name("dst_mac") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_MAC) @format(MAC_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @id(5) @name("dst_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @id(6) @name("dst_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            dscp                                            : ternary @id(7) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn                                             : ternary @id(10) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
            local_metadata.ingress_port                     : optional @id(8) @name("in_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IN_PORT);
        }
        actions = {
            @proto_id(1) set_vrf;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_pre_ingress_counter;
        size = 254;
    }
    @id(0x02000105) @sai_acl(PRE_INGRESS) @p4runtime_role("sdn_controller") @sai_acl_priority(1) @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
    // Disallow match on reserved VLAN IDs to rule out vendor specific behavior.
    vlan_id::mask != 0 -> (vlan_id != 4095 && vlan_id != 0);
    // TODO: Disallow setting to reserved VLAN IDs when supported.
  ") table acl_pre_ingress_vlan_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            local_metadata.vlan_id                          : ternary @id(5) @name("vlan_id") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID);
        }
        actions = {
            @proto_id(1) set_outer_vlan_id;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_pre_ingress_vlan_counter;
        size = 254;
    }
    @id(0x02000106) @sai_acl(PRE_INGRESS) @p4runtime_role("sdn_controller") @sai_acl_priority(5) @entry_restriction("
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // DSCP is only allowed on IP traffic.
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ecn::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
    // Only allow icmp_type matches for ICMP packets
    icmpv6_type::mask != 0 -> ip_protocol == 58;
  ") table acl_pre_ingress_metadata_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            ip_protocol                                     : ternary @id(4) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(5) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            dscp                                            : ternary @id(6) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn                                             : ternary @id(7) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
        }
        actions = {
            @proto_id(1) set_acl_metadata;
            @defaultonly NoAction;
        }
        const default_action = NoAction;
        counters = acl_pre_ingress_metadata_counter;
        size = 254;
    }
    apply {
        if (headers.ipv4.isValid()) {
            dscp = headers.ipv4.dscp;
            ecn = headers.ipv4.ecn;
            ip_protocol = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            dscp = headers.ipv6.dscp;
            ecn = headers.ipv6.ecn;
            ip_protocol = headers.ipv6.next_header;
        }
        acl_pre_ingress_table.apply();
    }
}

control admit_google_system_mac(in headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        local_metadata.admit_to_l3 = headers.ethernet.dst_addr == 0x1a11175f80;
    }
}

control hashing(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    bit<32> seed = 0;
    bit<8> offset = 0;
    @sai_hash_seed(0) @id(0x010000A) action select_ecmp_hash_algorithm() {
        seed = 0;
    }
    @sai_ecmp_hash(SAI_SWITCH_ATTR_ECMP_HASH_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @id(0x0100000B) action compute_ecmp_hash_ipv4() {
        hash(local_metadata.wcmp_selector_input, HashAlgorithm.crc32, 1w0, { seed, headers.ipv4.src_addr, headers.ipv4.dst_addr, local_metadata.l4_src_port, local_metadata.l4_dst_port }, 17w0x10000);
        local_metadata.wcmp_selector_input = local_metadata.wcmp_selector_input >> offset | local_metadata.wcmp_selector_input << 8 - offset;
    }
    @sai_ecmp_hash(SAI_SWITCH_ATTR_ECMP_HASH_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) @id(0x0100000C) action compute_ecmp_hash_ipv6() {
        hash(local_metadata.wcmp_selector_input, HashAlgorithm.crc32, 1w0, { seed, headers.ipv6.flow_label, headers.ipv6.src_addr, headers.ipv6.dst_addr, local_metadata.l4_src_port, local_metadata.l4_dst_port }, 17w0x10000);
        local_metadata.wcmp_selector_input = local_metadata.wcmp_selector_input >> offset | local_metadata.wcmp_selector_input << 8 - offset;
    }
    apply {
        select_ecmp_hash_algorithm();
        if (headers.ipv4.isValid()) {
            compute_ecmp_hash_ipv4();
        } else if (headers.ipv6.isValid()) {
            compute_ecmp_hash_ipv6();
        }
    }
}

control lag_hashing_config(in headers_t headers) {
    bit<32> lag_seed = 0;
    bit<4> lag_offset = 0;
    @sai_hash_algorithm(SAI_HASH_ALGORITHM_CRC) @sai_hash_seed(0) @sai_hash_offset(0) action select_lag_hash_algorithm() {
        lag_seed = 0;
        lag_offset = 0;
    }
    @sai_lag_hash(SAI_SWITCH_ATTR_LAG_HASH_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @id(0x0100000D) action compute_lag_hash_ipv4() {
    }
    @sai_lag_hash(SAI_SWITCH_ATTR_LAG_HASH_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) @id(0x0100000E) action compute_lag_hash_ipv6() {
    }
    apply {
        select_lag_hash_algorithm();
        if (headers.ipv4.isValid()) {
            compute_lag_hash_ipv4();
        } else if (headers.ipv6.isValid()) {
            compute_lag_hash_ipv6();
        }
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        packet_out_decap.apply(headers, local_metadata, standard_metadata);
        if (!local_metadata.bypass_ingress) {
            tunnel_termination_lookup.apply(headers, local_metadata);
            vlan_untag.apply(headers, local_metadata, standard_metadata);
            acl_pre_ingress.apply(headers, local_metadata, standard_metadata);
            ingress_vlan_checks.apply(headers, local_metadata, standard_metadata);
            tunnel_termination_decap.apply(headers, local_metadata);
            admit_google_system_mac.apply(headers, local_metadata);
            l3_admit.apply(headers, local_metadata, standard_metadata);
            hashing.apply(headers, local_metadata, standard_metadata);
            lag_hashing_config.apply(headers);
            routing_lookup.apply(headers, local_metadata, standard_metadata);
            acl_ingress.apply(headers, local_metadata, standard_metadata);
            routing_resolution.apply(headers, local_metadata, standard_metadata);
            mirror_session_lookup.apply(headers, local_metadata, standard_metadata);
            ingress_cloning.apply(headers, local_metadata, standard_metadata);
            drop_martians.apply(headers, local_metadata, standard_metadata);
        }
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        packet_in_encap.apply(headers, local_metadata, standard_metadata);
        if (!(standard_metadata.instance_type == 1 && standard_metadata.egress_rid == 1)) {
            packet_rewrites.apply(headers, local_metadata, standard_metadata);
            gre_tunnel_encap.apply(headers, local_metadata, standard_metadata);
            mirroring_encap.apply(headers, local_metadata, standard_metadata);
            egress_vlan_checks.apply(headers, local_metadata, standard_metadata);
            vlan_tag.apply(headers, local_metadata, standard_metadata);
            acl_egress.apply(headers, local_metadata, standard_metadata);
        }
    }
}

@pkginfo(name="fabric_border_router.p4", organization="Google", version="1.6.1") V1Switch(packet_parser(), verify_ipv4_checksum(), ingress(), egress(), compute_ipv4_checksum(), packet_deparser()) main;
