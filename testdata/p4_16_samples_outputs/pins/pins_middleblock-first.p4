#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
typedef bit<12> vlan_id_t;
typedef bit<16> ether_type_t;
const vlan_id_t INTERNAL_VLAN_ID_0 = 12w0xfff;
const vlan_id_t NO_VLAN_ID_0 = 12w0x0;
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
const vrf_id_t kDefaultVrf_0 = (vrf_id_t)10w0;
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

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        local_metadata.enable_vlan_checks = false;
        local_metadata.vlan_id = 12w0;
        local_metadata.admit_to_l3 = false;
        local_metadata.vrf_id = (vrf_id_t)10w0;
        local_metadata.enable_decrement_ttl = false;
        local_metadata.enable_src_mac_rewrite = false;
        local_metadata.enable_dst_mac_rewrite = false;
        local_metadata.enable_vlan_rewrite = false;
        local_metadata.packet_rewrites.src_mac = 48w0;
        local_metadata.packet_rewrites.dst_mac = 48w0;
        local_metadata.l4_src_port = 16w0;
        local_metadata.l4_dst_port = 16w0;
        local_metadata.wcmp_selector_input = 8w0;
        local_metadata.apply_tunnel_decap_at_end_of_pre_ingress = false;
        local_metadata.apply_tunnel_encap_at_egress = false;
        local_metadata.tunnel_encap_src_ipv6 = 128w0;
        local_metadata.tunnel_encap_dst_ipv6 = 128w0;
        local_metadata.marked_to_copy = false;
        local_metadata.marked_to_mirror = false;
        local_metadata.mirror_session_id = (mirror_session_id_t)10w0;
        local_metadata.mirror_egress_port = (port_id_t)9w0;
        local_metadata.color = MeterColor_t.GREEN;
        local_metadata.route_metadata = 6w0;
        local_metadata.bypass_ingress = false;
        local_metadata.wcmp_group_id_valid = false;
        local_metadata.wcmp_group_id_value = (wcmp_group_id_t)12w0;
        local_metadata.nexthop_id_valid = false;
        local_metadata.nexthop_id_value = (nexthop_id_t)10w0;
        local_metadata.ipmc_table_hit = false;
        local_metadata.acl_drop = false;
        transition select(standard_metadata.instance_type == 32w4) {
            true: start_true;
            false: start_false;
        }
    }
    state start_true {
        local_metadata.ingress_port = (port_id_t)local_metadata.loopback_port;
        transition start_join;
    }
    state start_false {
        local_metadata.ingress_port = (port_id_t)standard_metadata.ingress_port;
        transition start_join;
    }
    state start_join {
        transition select(standard_metadata.ingress_port) {
            9w510: parse_packet_out_header;
            default: parse_ethernet;
        }
    }
    state parse_packet_out_header {
        packet.extract<packet_out_header_t>(headers.packet_out_header);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp;
            16w0x8100: parse_8021q_vlan;
            default: accept;
        }
    }
    state parse_8021q_vlan {
        packet.extract<vlan_t>(headers.vlan);
        transition select(headers.vlan.ether_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(headers.ipv4);
        transition select(headers.ipv4.protocol) {
            8w0x4: parse_ipv4_in_ip;
            8w0x29: parse_ipv6_in_ip;
            8w0x1: parse_icmp;
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv4_in_ip {
        packet.extract<ipv4_t>(headers.inner_ipv4);
        transition select(headers.inner_ipv4.protocol) {
            8w0x1: parse_icmp;
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract<ipv6_t>(headers.ipv6);
        transition select(headers.ipv6.next_header) {
            8w0x4: parse_ipv4_in_ip;
            8w0x29: parse_ipv6_in_ip;
            8w0x3a: parse_icmp;
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6_in_ip {
        packet.extract<ipv6_t>(headers.inner_ipv6);
        transition select(headers.inner_ipv6.next_header) {
            8w0x3a: parse_icmp;
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<tcp_t>(headers.tcp);
        local_metadata.l4_src_port = headers.tcp.src_port;
        local_metadata.l4_dst_port = headers.tcp.dst_port;
        transition accept;
    }
    state parse_udp {
        packet.extract<udp_t>(headers.udp);
        local_metadata.l4_src_port = headers.udp.src_port;
        local_metadata.l4_dst_port = headers.udp.dst_port;
        transition accept;
    }
    state parse_icmp {
        packet.extract<icmp_t>(headers.icmp);
        transition accept;
    }
    state parse_arp {
        packet.extract<arp_t>(headers.arp);
        transition accept;
    }
}

control packet_deparser(packet_out packet, in headers_t headers) {
    apply {
        packet.emit<packet_out_header_t>(headers.packet_out_header);
        packet.emit<ethernet_t>(headers.mirror_encap_ethernet);
        packet.emit<vlan_t>(headers.mirror_encap_vlan);
        packet.emit<ipv6_t>(headers.mirror_encap_ipv6);
        packet.emit<udp_t>(headers.mirror_encap_udp);
        packet.emit<ipfix_t>(headers.ipfix);
        packet.emit<psamp_extended_t>(headers.psamp_extended);
        packet.emit<ethernet_t>(headers.ethernet);
        packet.emit<vlan_t>(headers.vlan);
        packet.emit<ipv6_t>(headers.tunnel_encap_ipv6);
        packet.emit<gre_t>(headers.tunnel_encap_gre);
        packet.emit<ipv4_t>(headers.ipv4);
        packet.emit<ipv6_t>(headers.ipv6);
        packet.emit<ipv4_t>(headers.inner_ipv4);
        packet.emit<ipv6_t>(headers.inner_ipv6);
        packet.emit<arp_t>(headers.arp);
        packet.emit<icmp_t>(headers.icmp);
        packet.emit<tcp_t>(headers.tcp);
        packet.emit<udp_t>(headers.udp);
    }
}

control packet_in_encap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control packet_out_decap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (headers.packet_out_header.isValid() && headers.packet_out_header.submit_to_ingress == 1w0) {
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
    @id(0x01798B9E) @name("no_action") action no_action_0() {
    }
    @entry_restriction("
    // The VRF ID 0 (or '' in P4Runtime) encodes the default VRF, which cannot
    // be read or written via this table, but is always present implicitly.
    // TODO: This constraint should read `vrf_id != ''` (since
    // constraints are a control plane (P4Runtime) concept), but
    // p4-constraints does not currently support strings.
    vrf_id != 0;
  ") @p4runtime_role("sdn_controller") @id(0x0200004A) @name("vrf_table") table vrf_table_0 {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id");
        }
        actions = {
            @proto_id(1) no_action_0();
        }
        const default_action = no_action_0();
        size = 64;
    }
    @id(0x01000006) @name("drop") action drop_0() {
        mark_to_drop(standard_metadata);
    }
    @id(0x01000004) @name("set_wcmp_group_id") action set_wcmp_group_id_0(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id) {
        local_metadata.wcmp_group_id_valid = true;
        local_metadata.wcmp_group_id_value = wcmp_group_id;
    }
    @id(0x01000011) @name("set_wcmp_group_id_and_metadata") action set_wcmp_group_id_and_metadata_0(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id, route_metadata_t route_metadata) {
        set_wcmp_group_id_0(wcmp_group_id);
        local_metadata.route_metadata = route_metadata;
    }
    @id(0x01000015) @name("set_metadata_and_drop") action set_metadata_and_drop_0(@id(1) route_metadata_t route_metadata) {
        local_metadata.route_metadata = route_metadata;
        mark_to_drop(standard_metadata);
    }
    @id(0x01000010) @name("set_nexthop_id_and_metadata") action set_nexthop_id_and_metadata_0(@id(1) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id, route_metadata_t route_metadata) {
        local_metadata.nexthop_id_valid = true;
        local_metadata.nexthop_id_value = nexthop_id;
        local_metadata.route_metadata = route_metadata;
    }
    @id(0x01000018) @action_restriction("
    // Disallow 0 since it encodes 'no multicast' in V1Model.
    multicast_group_id != 0;
  ") @name("set_multicast_group_id") action set_multicast_group_id_0(@id(1) @refers_to(builtin : : multicast_group_table , multicast_group_id) multicast_group_id_t multicast_group_id) {
        standard_metadata.mcast_grp = multicast_group_id;
    }
    @p4runtime_role("sdn_controller") @id(0x02000044) @name("ipv4_table") table ipv4_table_0 {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv4.dst_addr: lpm @id(2) @name("ipv4_dst") @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) drop_0();
            @proto_id(2) set_nexthop_id(local_metadata);
            @proto_id(3) set_wcmp_group_id_0();
            @proto_id(5) set_nexthop_id_and_metadata_0();
            @proto_id(6) set_wcmp_group_id_and_metadata_0();
            @proto_id(7) set_metadata_and_drop_0();
        }
        const default_action = drop_0();
        size = 131072;
    }
    @p4runtime_role("sdn_controller") @id(0x02000045) @name("ipv6_table") table ipv6_table_0 {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv6.dst_addr: lpm @id(2) @name("ipv6_dst") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) drop_0();
            @proto_id(2) set_nexthop_id(local_metadata);
            @proto_id(3) set_wcmp_group_id_0();
            @proto_id(5) set_nexthop_id_and_metadata_0();
            @proto_id(6) set_wcmp_group_id_and_metadata_0();
            @proto_id(7) set_metadata_and_drop_0();
        }
        const default_action = drop_0();
        size = 17000;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004E) @name("ipv4_multicast_table") table ipv4_multicast_table_0 {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv4.dst_addr: exact @id(2) @name("ipv4_dst") @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) set_multicast_group_id_0();
            @defaultonly NoAction();
        }
        size = 1600;
        default_action = NoAction();
    }
    @p4runtime_role("sdn_controller") @id(0x0200004F) @name("ipv6_multicast_table") table ipv6_multicast_table_0 {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv6.dst_addr: exact @id(2) @name("ipv6_dst") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) set_multicast_group_id_0();
            @defaultonly NoAction();
        }
        size = 1600;
        default_action = NoAction();
    }
    apply {
        mark_to_drop(standard_metadata);
        vrf_table_0.apply();
        if (local_metadata.admit_to_l3) {
            if (headers.ipv4.isValid()) {
                ipv4_table_0.apply();
                ipv4_multicast_table_0.apply();
                local_metadata.ipmc_table_hit = standard_metadata.mcast_grp != 16w0;
            } else if (headers.ipv6.isValid()) {
                ipv6_table_0.apply();
                ipv6_multicast_table_0.apply();
                local_metadata.ipmc_table_hit = standard_metadata.mcast_grp != 16w0;
            }
        }
    }
}

control routing_resolution(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("tunnel_id_valid") bool tunnel_id_valid_0 = false;
    @name("tunnel_id_value") tunnel_id_t tunnel_id_value_0;
    @name("router_interface_id_valid") bool router_interface_id_valid_0 = false;
    @name("router_interface_id_value") router_interface_id_t router_interface_id_value_0;
    @name("neighbor_id_valid") bool neighbor_id_valid_0 = false;
    @name("neighbor_id_value") ipv6_addr_t neighbor_id_value_0;
    @id(0x01000001) @name("set_dst_mac") action set_dst_mac_0(@id(1) @format(MAC_ADDRESS) ethernet_addr_t dst_mac) {
        local_metadata.packet_rewrites.dst_mac = dst_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x02000040) @name("neighbor_table") table neighbor_table_0 {
        key = {
            router_interface_id_value_0: exact @id(1) @name("router_interface_id") @refers_to(router_interface_table , router_interface_id);
            neighbor_id_value_0        : exact @id(2) @format(IPV6_ADDRESS) @name("neighbor_id");
        }
        actions = {
            @proto_id(1) set_dst_mac_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 1024;
    }
    @id(0x0100001B) @unsupported @action_restriction("
    // Disallow reserved VLAN IDs with implementation-defined semantics.
    vlan_id != 0 && vlan_id != 4095") @name("set_port_and_src_mac_and_vlan_id") action set_port_and_src_mac_and_vlan_id_0(@id(1) port_id_t port, @id(2) @format(MAC_ADDRESS) ethernet_addr_t src_mac, @id(3) vlan_id_t vlan_id) {
        standard_metadata.egress_spec = (bit<9>)port;
        local_metadata.packet_rewrites.src_mac = src_mac;
        local_metadata.packet_rewrites.vlan_id = vlan_id;
    }
    @id(0x01000002) @name("set_port_and_src_mac") action set_port_and_src_mac_0(@id(1) port_id_t port, @id(2) @format(MAC_ADDRESS) ethernet_addr_t src_mac) {
        set_port_and_src_mac_and_vlan_id_0(port, src_mac, 12w0xfff);
    }
    @p4runtime_role("sdn_controller") @id(0x02000041) @name("router_interface_table") table router_interface_table_0 {
        key = {
            router_interface_id_value_0: exact @id(1) @name("router_interface_id");
        }
        actions = {
            @proto_id(1) set_port_and_src_mac_0();
            @proto_id(2) set_port_and_src_mac_and_vlan_id_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 256;
    }
    @id(0x01000017) @name("set_ip_nexthop_and_disable_rewrites") action set_ip_nexthop_and_disable_rewrites_0(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id, @id(3) bit<1> disable_decrement_ttl, @id(4) bit<1> disable_src_mac_rewrite, @id(5) bit<1> disable_dst_mac_rewrite, @id(6) bit<1> disable_vlan_rewrite) {
        router_interface_id_valid_0 = true;
        router_interface_id_value_0 = router_interface_id;
        neighbor_id_valid_0 = true;
        neighbor_id_value_0 = neighbor_id;
        local_metadata.enable_decrement_ttl = !(bool)disable_decrement_ttl;
        local_metadata.enable_src_mac_rewrite = !(bool)disable_src_mac_rewrite;
        local_metadata.enable_dst_mac_rewrite = !(bool)disable_dst_mac_rewrite;
        local_metadata.enable_vlan_rewrite = !(bool)disable_vlan_rewrite;
    }
    @id(0x01000014) @name("set_ip_nexthop") action set_ip_nexthop_0(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        set_ip_nexthop_and_disable_rewrites_0(router_interface_id, neighbor_id, 1w0x0, 1w0x0, 1w0x0, 1w0x0);
    }
    @id(0x01000003) @deprecated("Use set_ip_nexthop instead.") @name("set_nexthop") action set_nexthop_0(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        set_ip_nexthop_0(router_interface_id, neighbor_id);
    }
    @id(0x01000012) @name("set_p2p_tunnel_encap_nexthop") action set_p2p_tunnel_encap_nexthop_0(@id(1) @refers_to(tunnel_table , tunnel_id) tunnel_id_t tunnel_id) {
        tunnel_id_valid_0 = true;
        tunnel_id_value_0 = tunnel_id;
    }
    @p4runtime_role("sdn_controller") @id(0x02000042) @name("nexthop_table") table nexthop_table_0 {
        key = {
            local_metadata.nexthop_id_value: exact @id(1) @name("nexthop_id");
        }
        actions = {
            @proto_id(1) set_nexthop_0();
            @proto_id(2) set_p2p_tunnel_encap_nexthop_0();
            @proto_id(3) set_ip_nexthop_0();
            @proto_id(4) set_ip_nexthop_and_disable_rewrites_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 1024;
    }
    @id(0x01000013) @name("mark_for_p2p_tunnel_encap") action mark_for_p2p_tunnel_encap_0(@id(1) @format(IPV6_ADDRESS) ipv6_addr_t encap_src_ip, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t encap_dst_ip, @id(3) @refers_to(neighbor_table , router_interface_id) @refers_to(router_interface_table , router_interface_id) router_interface_id_t router_interface_id) {
        local_metadata.tunnel_encap_src_ipv6 = encap_src_ip;
        local_metadata.tunnel_encap_dst_ipv6 = encap_dst_ip;
        local_metadata.apply_tunnel_encap_at_egress = true;
        set_ip_nexthop_0(router_interface_id, encap_dst_ip);
    }
    @p4runtime_role("sdn_controller") @id(0x02000050) @name("tunnel_table") table tunnel_table_0 {
        key = {
            tunnel_id_value_0: exact @id(1) @name("tunnel_id");
        }
        actions = {
            @proto_id(1) mark_for_p2p_tunnel_encap_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 2048;
    }
    @max_group_size(512) @id(0x11DC4EC8) @name("wcmp_group_selector") action_selector(HashAlgorithm.identity, 32w49152, 32w8) wcmp_group_selector_0;
    @p4runtime_role("sdn_controller") @id(0x02000043) @oneshot @name("wcmp_group_table") table wcmp_group_table_0 {
        key = {
            local_metadata.wcmp_group_id_value: exact @id(1) @name("wcmp_group_id");
            local_metadata.wcmp_selector_input: selector @name("local_metadata.wcmp_selector_input");
        }
        actions = {
            @proto_id(1) set_nexthop_id(local_metadata);
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        implementation = wcmp_group_selector_0;
        size = 3968;
    }
    apply {
        if (local_metadata.admit_to_l3) {
            if (local_metadata.wcmp_group_id_valid) {
                wcmp_group_table_0.apply();
            }
            if (local_metadata.nexthop_id_valid) {
                nexthop_table_0.apply();
                if (tunnel_id_valid_0) {
                    tunnel_table_0.apply();
                }
                if (router_interface_id_valid_0 && neighbor_id_valid_0) {
                    router_interface_table_0.apply();
                    neighbor_table_0.apply();
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
        verify_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<1>, bit<1>, bit<1>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(headers.ipv4.isValid(), { headers.ipv4.version, headers.ipv4.ihl, headers.ipv4.dscp, headers.ipv4.ecn, headers.ipv4.total_len, headers.ipv4.identification, headers.ipv4.reserved, headers.ipv4.do_not_fragment, headers.ipv4.more_fragments, headers.ipv4.frag_offset, headers.ipv4.ttl, headers.ipv4.protocol, headers.ipv4.src_addr, headers.ipv4.dst_addr }, headers.ipv4.header_checksum, HashAlgorithm.csum16);
    }
}

control compute_ipv4_checksum(inout headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<1>, bit<1>, bit<1>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(headers.ipv4.isValid(), { headers.ipv4.version, headers.ipv4.ihl, headers.ipv4.dscp, headers.ipv4.ecn, headers.ipv4.total_len, headers.ipv4.identification, headers.ipv4.reserved, headers.ipv4.do_not_fragment, headers.ipv4.more_fragments, headers.ipv4.frag_offset, headers.ipv4.ttl, headers.ipv4.protocol, headers.ipv4.src_addr, headers.ipv4.dst_addr }, headers.ipv4.header_checksum, HashAlgorithm.csum16);
    }
}

control ingress_cloning(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x0100001C) @name("ingress_clone") action ingress_clone_0(@id(1) bit<32> clone_session) {
        clone_preserving_field_list(CloneType.I2E, clone_session, PreservedFieldList.MIRROR_AND_PACKET_IN_COPY);
    }
    @unsupported @p4runtime_role("packet_replication_engine_manager") @id(0x02000051) @entry_restriction("
    // mirror_egress_port is present iff marked_to_mirror is true.
    // Exact match indicating presence of mirror_egress_port.
    marked_to_mirror == 1 -> mirror_egress_port::mask == -1;
    // Wildcard match indicating abscence of mirror_egress_port.
    marked_to_mirror == 0 -> mirror_egress_port::mask == 0;
  ") @name("ingress_clone_table") table ingress_clone_table_0 {
        key = {
            local_metadata.marked_to_copy    : exact @id(1) @name("marked_to_copy");
            local_metadata.marked_to_mirror  : exact @id(2) @name("marked_to_mirror");
            local_metadata.mirror_egress_port: optional @id(3) @name("mirror_egress_port");
        }
        actions = {
            @proto_id(1) ingress_clone_0();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        ingress_clone_table_0.apply();
    }
}

control mirror_session_lookup(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @id(0x01000007) @name("mirror_as_ipv4_erspan") action mirror_as_ipv4_erspan_0(@id(1) port_id_t port, @id(2) @format(IPV4_ADDRESS) ipv4_addr_t src_ip, @id(3) @format(IPV4_ADDRESS) ipv4_addr_t dst_ip, @id(4) @format(MAC_ADDRESS) ethernet_addr_t src_mac, @id(5) @format(MAC_ADDRESS) ethernet_addr_t dst_mac, @id(6) bit<8> ttl, @id(7) bit<8> tos) {
    }
    @id(0x0100001D) @unsupported @name("mirror_with_vlan_tag_and_ipfix_encapsulation") action mirror_with_vlan_tag_and_ipfix_encapsulation_0(@id(1) port_id_t monitor_port, @id(2) port_id_t monitor_failover_port, @id(3) @format(MAC_ADDRESS) ethernet_addr_t mirror_encap_src_mac, @id(4) @format(MAC_ADDRESS) ethernet_addr_t mirror_encap_dst_mac, @id(6) vlan_id_t mirror_encap_vlan_id, @id(7) @format(IPV6_ADDRESS) ipv6_addr_t mirror_encap_dst_ip, @id(8) @format(IPV6_ADDRESS) ipv6_addr_t mirror_encap_src_ip, @id(9) bit<16> mirror_encap_udp_src_port, @id(10) bit<16> mirror_encap_udp_dst_port) {
        local_metadata.mirror_egress_port = monitor_port;
        local_metadata.mirror_encap_src_mac = mirror_encap_src_mac;
        local_metadata.mirror_encap_dst_mac = mirror_encap_dst_mac;
        local_metadata.mirror_encap_vlan_id = mirror_encap_vlan_id;
        local_metadata.mirror_encap_src_ip = mirror_encap_src_ip;
        local_metadata.mirror_encap_dst_ip = mirror_encap_dst_ip;
        local_metadata.mirror_encap_udp_src_port = mirror_encap_udp_src_port;
        local_metadata.mirror_encap_udp_dst_port = mirror_encap_udp_dst_port;
    }
    @p4runtime_role("sdn_controller") @id(0x02000046) @name("mirror_session_table") table mirror_session_table_0 {
        key = {
            local_metadata.mirror_session_id: exact @id(1) @name("mirror_session_id");
        }
        actions = {
            @proto_id(1) mirror_as_ipv4_erspan_0();
            @proto_id(2) mirror_with_vlan_tag_and_ipfix_encapsulation_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 4;
    }
    apply {
        if (local_metadata.marked_to_mirror) {
            mirror_session_table_0.apply();
        }
    }
}

control mirroring_encap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.instance_type == 32w1 && standard_metadata.egress_rid == 16w2) {
            headers.mirror_encap_ethernet.setValid();
            headers.mirror_encap_ethernet.src_addr = local_metadata.mirror_encap_src_mac;
            headers.mirror_encap_ethernet.dst_addr = local_metadata.mirror_encap_dst_mac;
            headers.mirror_encap_ethernet.ether_type = 16w0x8100;
            headers.mirror_encap_vlan.setValid();
            headers.mirror_encap_vlan.ether_type = 16w0x86dd;
            headers.mirror_encap_vlan.vlan_id = local_metadata.mirror_encap_vlan_id;
            headers.mirror_encap_ipv6.setValid();
            headers.mirror_encap_ipv6.version = 4w6;
            headers.mirror_encap_ipv6.dscp = 6w0;
            headers.mirror_encap_ipv6.ecn = 2w0;
            headers.mirror_encap_ipv6.hop_limit = 8w16;
            headers.mirror_encap_ipv6.flow_label = 20w0;
            headers.mirror_encap_ipv6.payload_length = (bit<16>)standard_metadata.packet_length + 16w52;
            headers.mirror_encap_ipv6.next_header = 8w0x11;
            headers.mirror_encap_ipv6.src_addr = local_metadata.mirror_encap_src_ip;
            headers.mirror_encap_ipv6.dst_addr = local_metadata.mirror_encap_dst_ip;
            headers.mirror_encap_udp.setValid();
            headers.mirror_encap_udp.src_port = local_metadata.mirror_encap_udp_src_port;
            headers.mirror_encap_udp.dst_port = local_metadata.mirror_encap_udp_dst_port;
            headers.mirror_encap_udp.hdr_length = headers.mirror_encap_ipv6.payload_length;
            headers.mirror_encap_udp.checksum = 16w0;
            headers.ipfix.setValid();
            headers.psamp_extended.setValid();
        }
    }
}

control l3_admit(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @id(0x01000008) @name("admit_to_l3") action admit_to_l3_0() {
        local_metadata.admit_to_l3 = true;
    }
    @p4runtime_role("sdn_controller") @id(0x02000047) @name("l3_admit_table") table l3_admit_table_0 {
        key = {
            headers.ethernet.dst_addr  : ternary @name("dst_mac") @id(1) @format(MAC_ADDRESS);
            local_metadata.ingress_port: optional @name("in_port") @id(2);
        }
        actions = {
            @proto_id(1) admit_to_l3_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 64;
    }
    apply {
        if (local_metadata.enable_vlan_checks && !(local_metadata.vlan_id == 12w0x0 || local_metadata.vlan_id == 12w0xfff)) {
            local_metadata.admit_to_l3 = false;
        } else {
            l3_admit_table_0.apply();
        }
    }
}

control gre_tunnel_encap(inout headers_t headers, in local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.apply_tunnel_encap_at_egress) {
            headers.tunnel_encap_gre.setValid();
            headers.tunnel_encap_gre.checksum_present = 1w0;
            headers.tunnel_encap_gre.routing_present = 1w0;
            headers.tunnel_encap_gre.key_present = 1w0;
            headers.tunnel_encap_gre.sequence_present = 1w0;
            headers.tunnel_encap_gre.strict_source_route = 1w0;
            headers.tunnel_encap_gre.recursion_control = 3w0;
            headers.tunnel_encap_gre.flags = 4w0;
            headers.tunnel_encap_gre.version = 3w0;
            headers.tunnel_encap_gre.protocol = headers.ethernet.ether_type;
            headers.ethernet.ether_type = 16w0x86dd;
            headers.tunnel_encap_ipv6.setValid();
            headers.tunnel_encap_ipv6.version = 4w6;
            headers.tunnel_encap_ipv6.src_addr = local_metadata.tunnel_encap_src_ipv6;
            headers.tunnel_encap_ipv6.dst_addr = local_metadata.tunnel_encap_dst_ipv6;
            headers.tunnel_encap_ipv6.payload_length = (bit<16>)standard_metadata.packet_length + 16w65526;
            headers.tunnel_encap_ipv6.next_header = 8w0x2f;
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
    @id(0x0100001A) @name("disable_vlan_checks") action disable_vlan_checks_0() {
        local_metadata.enable_vlan_checks = false;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004D) @entry_restriction("
    // Force the dummy_match to be wildcard.
    dummy_match::mask == 0;
  ") @name("disable_vlan_checks_table") table disable_vlan_checks_table_0 {
        key = {
            1w1: ternary @id(1) @name("dummy_match");
        }
        actions = {
            @proto_id(1) disable_vlan_checks_0();
            @defaultonly NoAction();
        }
        size = 1;
        default_action = NoAction();
    }
    apply {
        if (headers.vlan.isValid()) {
            local_metadata.vlan_id = headers.vlan.vlan_id;
            headers.ethernet.ether_type = headers.vlan.ether_type;
            headers.vlan.setInvalid();
        } else {
            local_metadata.vlan_id = 12w0xfff;
        }
        local_metadata.enable_vlan_checks = true;
        disable_vlan_checks_table_0.apply();
    }
}

control ingress_vlan_checks(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.enable_vlan_checks && !(local_metadata.vlan_id == 12w0x0 || local_metadata.vlan_id == 12w0xfff)) {
            mark_to_drop(standard_metadata);
        }
    }
}

control egress_vlan_checks(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.enable_vlan_checks) {
            if (standard_metadata.instance_type == 32w1 && standard_metadata.egress_rid == 16w2 && !(local_metadata.mirror_encap_vlan_id == 12w0x0 || local_metadata.mirror_encap_vlan_id == 12w0xfff)) {
                mark_to_drop(standard_metadata);
            } else if (!(standard_metadata.instance_type == 32w1 && standard_metadata.egress_rid == 16w1) && !(local_metadata.vlan_id == 12w0x0 || local_metadata.vlan_id == 12w0xfff)) {
                mark_to_drop(standard_metadata);
            }
        }
    }
}

control vlan_tag(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (!(local_metadata.vlan_id == 12w0x0 || local_metadata.vlan_id == 12w0xfff) && !(standard_metadata.instance_type == 32w1 && standard_metadata.egress_rid == 16w2)) {
            headers.vlan.setValid();
            headers.vlan.priority_code_point = 3w0;
            headers.vlan.drop_eligible_indicator = 1w0;
            headers.vlan.vlan_id = local_metadata.vlan_id;
            headers.vlan.ether_type = headers.ethernet.ether_type;
            headers.ethernet.ether_type = 16w0x8100;
        }
    }
}

const ipv6_addr_t IPV6_MULTICAST_MASK_0 = 128w0xff000000000000000000000000000000;
const ipv6_addr_t IPV6_MULTICAST_VALUE_0 = 128w0xff000000000000000000000000000000;
const ipv6_addr_t IPV6_LOOPBACK_MASK_0 = 128w0xffffffffffffffffffffffffffffffff;
const ipv6_addr_t IPV6_LOOPBACK_VALUE_0 = 128w0x1;
const ipv4_addr_t IPV4_MULTICAST_MASK_0 = 32w0xf0000000;
const ipv4_addr_t IPV4_MULTICAST_VALUE_0 = 32w0xe0000000;
const ipv4_addr_t IPV4_BROADCAST_VALUE_0 = 32w0xffffffff;
const ipv4_addr_t IPV4_LOOPBACK_MASK_0 = 32w0xff000000;
const ipv4_addr_t IPV4_LOOPBACK_VALUE_0 = 32w0x7f000000;
const ethernet_addr_t MAC_MULTICAST_MASK_0 = 48w0x10000000000;
const ethernet_addr_t MAC_MULTICAST_VALUE_0 = 48w0x10000000000;
control drop_martians(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (headers.ipv6.isValid() && (headers.ipv6.src_addr & 128w0xff000000000000000000000000000000 == 128w0xff000000000000000000000000000000 || headers.ipv6.dst_addr & 128w0xff000000000000000000000000000000 == 128w0xff000000000000000000000000000000 || headers.ipv6.src_addr == 128w0x1 || headers.ipv6.dst_addr == 128w0x1) || headers.ipv4.isValid() && (headers.ipv4.src_addr & 32w0xf0000000 == 32w0xe0000000 || headers.ipv4.src_addr == 32w0xffffffff || (headers.ipv4.dst_addr & 32w0xf0000000 == 32w0xe0000000 || headers.ipv4.dst_addr == 32w0xffffffff) || headers.ipv4.src_addr & 32w0xff000000 == 32w0x7f000000 || headers.ipv4.dst_addr & 32w0xff000000 == 32w0x7f000000) || headers.ethernet.isValid() && headers.ethernet.dst_addr & 48w0x10000000000 == 48w0x10000000000) {
            mark_to_drop(standard_metadata);
        }
    }
}

control multicast_rewrites(inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @name("multicast_replica_port") port_id_t multicast_replica_port_0 = (port_id_t)standard_metadata.egress_port;
    @name("multicast_replica_instance") replica_instance_t multicast_replica_instance_0 = standard_metadata.egress_rid;
    @id(0x01000019) @name("set_multicast_src_mac") action set_multicast_src_mac_0(@id(1) @format(MAC_ADDRESS) ethernet_addr_t src_mac) {
        local_metadata.enable_src_mac_rewrite = true;
        local_metadata.packet_rewrites.src_mac = src_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x0200004C) @name("multicast_router_interface_table") table multicast_router_interface_table_0 {
        key = {
            multicast_replica_port_0    : exact @referenced_by(builtin : : multicast_group_table , replica . port) @id(1) @name("multicast_replica_port");
            multicast_replica_instance_0: exact @referenced_by(builtin : : multicast_group_table , replica . instance) @id(2) @name("multicast_replica_instance");
        }
        actions = {
            @proto_id(1) set_multicast_src_mac_0();
            @defaultonly NoAction();
        }
        size = 110;
        default_action = NoAction();
    }
    apply {
        multicast_router_interface_table_0.apply();
    }
}

control packet_rewrites(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("multicast_rewrites") multicast_rewrites() multicast_rewrites_inst_0;
    apply {
        if (standard_metadata.instance_type == 32w5) {
            local_metadata.enable_decrement_ttl = true;
            multicast_rewrites_inst_0.apply(local_metadata, standard_metadata);
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
            if (headers.ipv4.ttl > 8w0 && local_metadata.enable_decrement_ttl) {
                headers.ipv4.ttl = headers.ipv4.ttl + 8w255;
            }
            if (headers.ipv4.ttl == 8w0) {
                mark_to_drop(standard_metadata);
            }
        }
        if (headers.ipv6.isValid()) {
            if (headers.ipv6.hop_limit > 8w0 && local_metadata.enable_decrement_ttl) {
                headers.ipv6.hop_limit = headers.ipv6.hop_limit + 8w255;
            }
            if (headers.ipv6.hop_limit == 8w0) {
                mark_to_drop(standard_metadata);
            }
        }
    }
}

control tunnel_termination_lookup(in headers_t headers, inout local_metadata_t local_metadata) {
    @id(0x01000016) @name("mark_for_tunnel_decap_and_set_vrf") action mark_for_tunnel_decap_and_set_vrf_0(@refers_to(vrf_table , vrf_id) vrf_id_t vrf_id) {
        local_metadata.apply_tunnel_decap_at_end_of_pre_ingress = true;
        local_metadata.vrf_id = vrf_id;
    }
    @unsupported @p4runtime_role("sdn_controller") @id(0x0200004B) @name("ipv6_tunnel_termination_table") table ipv6_tunnel_termination_table_0 {
        key = {
            headers.ipv6.dst_addr: ternary @id(1) @name("dst_ipv6") @format(IPV6_ADDRESS);
            headers.ipv6.src_addr: ternary @id(2) @name("src_ipv6") @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) mark_for_tunnel_decap_and_set_vrf_0();
            @defaultonly NoAction();
        }
        size = 126;
        default_action = NoAction();
    }
    apply {
        if (headers.ipv6.isValid()) {
            if (headers.ipv6.next_header == 8w0x4 || headers.ipv6.next_header == 8w0x29) {
                ipv6_tunnel_termination_table_0.apply();
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
                headers.ethernet.ether_type = 16w0x800;
                headers.ipv4 = headers.inner_ipv4;
                headers.inner_ipv4.setInvalid();
            }
            if (headers.inner_ipv6.isValid()) {
                headers.ethernet.ether_type = 16w0x86dd;
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
    @name("dscp") bit<6> dscp_0 = 6w0;
    @name("ip_protocol") bit<8> ip_protocol_0 = 8w0;
    @id(0x13000104) @name("acl_egress_counter") direct_counter(CounterType.packets_and_bytes) acl_egress_counter_0;
    @id(0x13000108) @name("acl_egress_dhcp_to_host_counter") direct_counter(CounterType.packets_and_bytes) acl_egress_dhcp_to_host_counter_0;
    @id(0x0100010D) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("acl_egress_forward") action acl_egress_forward_0() {
        acl_egress_counter_0.count();
    }
    @p4runtime_role("sdn_controller") @id(0x02000104) @sai_acl(EGRESS) @entry_restriction("





    // Only allow IP field matches for IP packets.
    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);







    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") @name("acl_egress_table") table acl_egress_table_0 {
        key = {
            ip_protocol_0                                   : ternary @id(2) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            (port_id_t)standard_metadata.egress_port        : optional @id(4) @name("out_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT);
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(5) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(6) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(7) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.src_addr                       : ternary @id(10) @name("src_mac") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC) @format(MAC_ADDRESS);
        }
        actions = {
            @proto_id(1) acl_drop(local_metadata);
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_egress_counter_0;
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
  ") @name("acl_egress_dhcp_to_host_table") table acl_egress_dhcp_to_host_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            ip_protocol_0                                   : ternary @id(5) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            local_metadata.l4_dst_port                      : ternary @id(6) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            (port_id_t)standard_metadata.egress_port        : optional @id(7) @name("out_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUT_PORT);
        }
        actions = {
            @proto_id(1) acl_drop(local_metadata);
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_egress_dhcp_to_host_counter_0;
        size = 127;
    }
    apply {
        if (headers.ipv4.isValid()) {
            dscp_0 = headers.ipv4.dscp;
            ip_protocol_0 = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            dscp_0 = headers.ipv6.dscp;
            ip_protocol_0 = headers.ipv6.next_header;
        } else {
            ip_protocol_0 = 8w0;
        }
        acl_egress_table_0.apply();
        if (local_metadata.acl_drop) {
            mark_to_drop(standard_metadata);
        }
    }
}

control acl_ingress(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("ttl") bit<8> ttl_0 = 8w0;
    @name("dscp") bit<6> dscp_1 = 6w0;
    @name("ecn") bit<2> ecn_0 = 2w0;
    @name("ip_protocol") bit<8> ip_protocol_1 = 8w0;
    @name("cancel_copy") bool cancel_copy_0 = false;
    @id(0x15000100) @mode(single_rate_two_color) @name("acl_ingress_meter") direct_meter<MeterColor_t>(MeterType.bytes) acl_ingress_meter_0;
    @id(0x15000102) @mode(single_rate_two_color) @name("acl_ingress_qos_meter") direct_meter<MeterColor_t>(MeterType.bytes) acl_ingress_qos_meter_0;
    @id(0x13000102) @name("acl_ingress_counter") direct_counter(CounterType.packets_and_bytes) acl_ingress_counter_0;
    @id(0x13000107) @name("acl_ingress_qos_counter") direct_counter(CounterType.packets_and_bytes) acl_ingress_qos_counter_0;
    @id(0x13000109) @name("acl_ingress_counting_counter") direct_counter(CounterType.packets_and_bytes) acl_ingress_counting_counter_0;
    @id(0x1300010A) @name("acl_ingress_security_counter") direct_counter(CounterType.packets_and_bytes) acl_ingress_security_counter_0;
    @id(0x01000101) @sai_action(SAI_PACKET_ACTION_COPY , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_RED) @name("acl_copy") action acl_copy_0(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_ingress_counter_0.count();
        acl_ingress_meter_0.read(local_metadata.color);
        local_metadata.marked_to_copy = true;
    }
    @id(0x01000102) @sai_action(SAI_PACKET_ACTION_TRAP , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) @name("acl_trap") action acl_trap_0(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_copy_0(qos_queue);
        local_metadata.acl_drop = true;
    }
    @id(0x01000103) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) @name("acl_forward") action acl_forward_0() {
        acl_ingress_meter_0.read(local_metadata.color);
    }
    @id(0x01000105) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("acl_count") action acl_count_0() {
        acl_ingress_counting_counter_0.count();
    }
    @id(0x01000104) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("acl_mirror") action acl_mirror_0(@id(1) @refers_to(mirror_session_table , mirror_session_id) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS) mirror_session_id_t mirror_session_id) {
        acl_ingress_counter_0.count();
        local_metadata.marked_to_mirror = true;
        local_metadata.mirror_session_id = mirror_session_id;
    }
    @id(0x0100010C) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_COPY_CANCEL , SAI_PACKET_COLOR_RED) @name("set_qos_queue_and_cancel_copy_above_rate_limit") action set_qos_queue_and_cancel_copy_above_rate_limit_0(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t qos_queue) {
        acl_ingress_qos_meter_0.read(local_metadata.color);
    }
    @id(0x01000111) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DENY , SAI_PACKET_COLOR_RED) @unsupported @name("set_cpu_and_multicast_queues_and_deny_above_rate_limit") action set_cpu_and_multicast_queues_and_deny_above_rate_limit_0(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue, @id(2) @sai_action_param(MULTICAST_QOS_QUEUE , SAI_PACKET_COLOR_GREEN) qos_queue_t green_multicast_queue, @id(3) @sai_action_param(MULTICAST_QOS_QUEUE , SAI_PACKET_COLOR_RED) qos_queue_t red_multicast_queue) {
        acl_ingress_qos_meter_0.read(local_metadata.color);
    }
    @id(0x0100010E) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DENY , SAI_PACKET_COLOR_RED) @name("set_cpu_queue_and_deny_above_rate_limit") action set_cpu_queue_and_deny_above_rate_limit_0(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue) {
        acl_ingress_qos_meter_0.read(local_metadata.color);
    }
    @id(0x01000110) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("set_cpu_queue") action set_cpu_queue_0(@id(1) @sai_action_param(QOS_QUEUE) qos_queue_t cpu_queue) {
    }
    @id(0x0100010F) @sai_action(SAI_PACKET_ACTION_DENY) @name("acl_deny") action acl_deny_0() {
        cancel_copy_0 = true;
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

    // Only allow arp_tpa matches for ARP packets.
    arp_tpa::mask != 0 -> ether_type == 0x0806;





    icmpv6_type::mask != 0 -> ip_protocol == 58;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") @name("acl_ingress_table") table acl_ingress_table_0 {
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
            ttl_0                                           : ternary @id(10) @name("ttl") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            dscp_1                                          : ternary @id(11) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn_0                                           : ternary @id(12) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
            ip_protocol_1                                   : ternary @id(13) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(14) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            local_metadata.l4_src_port                      : ternary @id(20) @name("l4_src_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT);
            local_metadata.l4_dst_port                      : ternary @id(15) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            headers.arp.target_proto_addr                   : ternary @id(16) @name("arp_tpa") @composite_field(@ sai_udf ( base = SAI_UDF_BASE_L3 , offset = 24 , length = 2 ) , @ sai_udf ( base = SAI_UDF_BASE_L3 , offset = 26 , length = 2 )) @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) acl_copy_0();
            @proto_id(2) acl_trap_0();
            @proto_id(3) acl_forward_0();
            @proto_id(4) acl_mirror_0();
            @proto_id(5) acl_drop(local_metadata);
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        meters = acl_ingress_meter_0;
        counters = acl_ingress_counter_0;
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
# 369 " .. / .. / .. / .. / pins - infra / sai_p4 / instantiations / google / acl_ingress . p4 "
  ") @name("acl_ingress_qos_table") table acl_ingress_qos_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            ttl_0                                           : ternary @id(7) @name("ttl") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            ip_protocol_1                                   : ternary @id(8) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(9) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            local_metadata.l4_dst_port                      : ternary @id(10) @name("l4_dst_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
        }
        actions = {
            @proto_id(1) set_qos_queue_and_cancel_copy_above_rate_limit_0();
            @proto_id(2) set_cpu_queue_and_deny_above_rate_limit_0();
            @proto_id(3) acl_forward_0();
            @proto_id(4) acl_drop(local_metadata);
            @proto_id(5) set_cpu_queue_0();
            @proto_id(6) set_cpu_and_multicast_queues_and_deny_above_rate_limit_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        meters = acl_ingress_qos_meter_0;
        counters = acl_ingress_qos_counter_0;
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
  ") @name("acl_ingress_counting_table") table acl_ingress_counting_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            dscp_1                                          : ternary @id(11) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            local_metadata.route_metadata                   : ternary @id(18) @name("route_metadata") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ROUTE_DST_USER_META);
        }
        actions = {
            @proto_id(3) acl_count_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_ingress_counting_counter_0;
        size = 255;
    }
    @id(0x01000112) @name("redirect_to_nexthop") action redirect_to_nexthop_0(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT) @sai_action_param_object_type(SAI_OBJECT_TYPE_NEXT_HOP) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id) {
        local_metadata.nexthop_id_valid = true;
        local_metadata.nexthop_id_value = nexthop_id;
        local_metadata.wcmp_group_id_valid = false;
        standard_metadata.mcast_grp = 16w0;
    }
    @id(0x01000113) @action_restriction("
    // Disallow 0 since it encodes 'no multicast' in V1Model.
    multicast_group_id != 0;
  ") @name("redirect_to_ipmc_group") action redirect_to_ipmc_group_0(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_REDIRECT) @sai_action_param_object_type(SAI_OBJECT_TYPE_IPMC_GROUP) @refers_to(builtin : : multicast_group_table , multicast_group_id) multicast_group_id_t multicast_group_id) {
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
  ") @name("acl_ingress_mirror_and_redirect_table") table acl_ingress_mirror_and_redirect_table_0 {
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
            @proto_id(4) acl_forward_0();
            @proto_id(1) acl_mirror_0();
            @proto_id(2) redirect_to_nexthop_0();
            @proto_id(3) redirect_to_ipmc_group_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 255;
    }
    @id(0x0200010A) @sai_acl(INGRESS) @sai_acl_priority(20) @p4runtime_role("sdn_controller") @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;

    // Only allow IP field matches for IP packets.
    dst_ip::mask != 0 -> is_ipv4 == 1;
    src_ip::mask != 0 -> is_ipv4 == 1;
    src_ipv6::mask != 0 -> is_ipv6 == 1;

  // TODO: This comment is required for the preprocessor to not
  // spit out nonsense.







    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") @name("acl_ingress_security_table") table acl_ingress_security_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            headers.ipv4.src_addr                           : ternary @id(5) @name("src_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_IP) @format(IPV4_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @id(6) @name("dst_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.src_addr[127:64]                   : ternary @id(7) @name("src_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2 )) @format(IPV6_ADDRESS);
        }
        actions = {
            @proto_id(1) acl_forward_0();
            @proto_id(2) acl_drop(local_metadata);
            @proto_id(3) acl_deny_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_ingress_security_counter_0;
        size = 255;
    }
    apply {
        if (headers.ipv4.isValid()) {
            ttl_0 = headers.ipv4.ttl;
            dscp_1 = headers.ipv4.dscp;
            ecn_0 = headers.ipv4.ecn;
            ip_protocol_1 = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            ttl_0 = headers.ipv6.hop_limit;
            dscp_1 = headers.ipv6.dscp;
            ecn_0 = headers.ipv6.ecn;
            ip_protocol_1 = headers.ipv6.next_header;
        }
        acl_ingress_table_0.apply();
        acl_ingress_mirror_and_redirect_table_0.apply();
        acl_ingress_security_table_0.apply();
        if (cancel_copy_0) {
            local_metadata.marked_to_copy = false;
        }
    }
}

control acl_pre_ingress(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @name("dscp") bit<6> dscp_2 = 6w0;
    @name("ecn") bit<2> ecn_1 = 2w0;
    @name("ip_protocol") bit<8> ip_protocol_2 = 8w0;
    @id(0x13000101) @name("acl_pre_ingress_counter") direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_counter_0;
    @id(0x13000106) @name("acl_pre_ingress_vlan_counter") direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_vlan_counter_0;
    @id(0x13000105) @name("acl_pre_ingress_metadata_counter") direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_metadata_counter_0;
    @id(0x01000100) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("set_vrf") action set_vrf_0(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_VRF) @refers_to(vrf_table , vrf_id) @id(1) vrf_id_t vrf_id) {
        local_metadata.vrf_id = vrf_id;
        acl_pre_ingress_counter_0.count();
    }
    @id(0x0100010A) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("set_outer_vlan_id") action set_outer_vlan_id_0(@id(1) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_OUTER_VLAN_ID) vlan_id_t vlan_id) {
        local_metadata.vlan_id = vlan_id;
        acl_pre_ingress_vlan_counter_0.count();
    }
    @id(0x0100010B) @sai_action(SAI_PACKET_ACTION_FORWARD) @name("set_acl_metadata") action set_acl_metadata_0(@id(1) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_ACL_META_DATA) acl_metadata_t acl_metadata) {
        local_metadata.acl_metadata = acl_metadata;
        acl_pre_ingress_metadata_counter_0.count();
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
  ") @name("acl_pre_ingress_table") table acl_pre_ingress_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.src_addr                       : ternary @id(4) @name("src_mac") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC) @format(MAC_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @id(5) @name("dst_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @id(6) @name("dst_ipv6") @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            dscp_2                                          : ternary @id(7) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn_1                                           : ternary @id(10) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
            local_metadata.ingress_port                     : optional @id(8) @name("in_port") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IN_PORT);
        }
        actions = {
            @proto_id(1) set_vrf_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_pre_ingress_counter_0;
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
  ") @name("acl_pre_ingress_vlan_table") table acl_pre_ingress_vlan_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @id(4) @name("ether_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            local_metadata.vlan_id                          : ternary @id(5) @name("vlan_id") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_OUTER_VLAN_ID);
        }
        actions = {
            @proto_id(1) set_outer_vlan_id_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_pre_ingress_vlan_counter_0;
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
  ") @name("acl_pre_ingress_metadata_table") table acl_pre_ingress_metadata_table_0 {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @id(1) @name("is_ip") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @id(2) @name("is_ipv4") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @id(3) @name("is_ipv6") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            ip_protocol_2                                   : ternary @id(4) @name("ip_protocol") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @id(5) @name("icmpv6_type") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            dscp_2                                          : ternary @id(6) @name("dscp") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn_1                                           : ternary @id(7) @name("ecn") @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
        }
        actions = {
            @proto_id(1) set_acl_metadata_0();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_pre_ingress_metadata_counter_0;
        size = 254;
    }
    apply {
        if (headers.ipv4.isValid()) {
            dscp_2 = headers.ipv4.dscp;
            ecn_1 = headers.ipv4.ecn;
            ip_protocol_2 = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            dscp_2 = headers.ipv6.dscp;
            ecn_1 = headers.ipv6.ecn;
            ip_protocol_2 = headers.ipv6.next_header;
        }
        acl_pre_ingress_table_0.apply();
    }
}

control admit_google_system_mac(in headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        local_metadata.admit_to_l3 = headers.ethernet.dst_addr == 48w0x1a11175f80;
    }
}

control hashing(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @name("seed") bit<32> seed_0 = 32w0;
    @name("offset") bit<8> offset_0 = 8w0;
    @sai_hash_seed(0) @id(0x010000A) @name("select_ecmp_hash_algorithm") action select_ecmp_hash_algorithm_0() {
        seed_0 = 32w0;
    }
    @sai_ecmp_hash(SAI_SWITCH_ATTR_ECMP_HASH_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @id(0x0100000B) @name("compute_ecmp_hash_ipv4") action compute_ecmp_hash_ipv4_0() {
        hash<bit<8>, bit<1>, tuple<bit<32>, bit<32>, bit<32>, bit<16>, bit<16>>, bit<17>>(local_metadata.wcmp_selector_input, HashAlgorithm.crc32, 1w0, { seed_0, headers.ipv4.src_addr, headers.ipv4.dst_addr, local_metadata.l4_src_port, local_metadata.l4_dst_port }, 17w0x10000);
        local_metadata.wcmp_selector_input = local_metadata.wcmp_selector_input >> offset_0 | local_metadata.wcmp_selector_input << 8w8 - offset_0;
    }
    @sai_ecmp_hash(SAI_SWITCH_ATTR_ECMP_HASH_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) @id(0x0100000C) @name("compute_ecmp_hash_ipv6") action compute_ecmp_hash_ipv6_0() {
        hash<bit<8>, bit<1>, tuple<bit<32>, bit<20>, bit<128>, bit<128>, bit<16>, bit<16>>, bit<17>>(local_metadata.wcmp_selector_input, HashAlgorithm.crc32, 1w0, { seed_0, headers.ipv6.flow_label, headers.ipv6.src_addr, headers.ipv6.dst_addr, local_metadata.l4_src_port, local_metadata.l4_dst_port }, 17w0x10000);
        local_metadata.wcmp_selector_input = local_metadata.wcmp_selector_input >> offset_0 | local_metadata.wcmp_selector_input << 8w8 - offset_0;
    }
    apply {
        select_ecmp_hash_algorithm_0();
        if (headers.ipv4.isValid()) {
            compute_ecmp_hash_ipv4_0();
        } else if (headers.ipv6.isValid()) {
            compute_ecmp_hash_ipv6_0();
        }
    }
}

control lag_hashing_config(in headers_t headers) {
    @name("lag_seed") bit<32> lag_seed_0 = 32w0;
    @name("lag_offset") bit<4> lag_offset_0 = 4w0;
    @sai_hash_algorithm(SAI_HASH_ALGORITHM_CRC) @sai_hash_seed(0) @sai_hash_offset(0) @name("select_lag_hash_algorithm") action select_lag_hash_algorithm_0() {
        lag_seed_0 = 32w0;
        lag_offset_0 = 4w0;
    }
    @sai_lag_hash(SAI_SWITCH_ATTR_LAG_HASH_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV4) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @id(0x0100000D) @name("compute_lag_hash_ipv4") action compute_lag_hash_ipv4_0() {
    }
    @sai_lag_hash(SAI_SWITCH_ATTR_LAG_HASH_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_SRC_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_DST_IPV6) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_L4_DST_PORT) @sai_native_hash_field(SAI_NATIVE_HASH_FIELD_IPV6_FLOW_LABEL) @id(0x0100000E) @name("compute_lag_hash_ipv6") action compute_lag_hash_ipv6_0() {
    }
    apply {
        select_lag_hash_algorithm_0();
        if (headers.ipv4.isValid()) {
            compute_lag_hash_ipv4_0();
        } else if (headers.ipv6.isValid()) {
            compute_lag_hash_ipv6_0();
        }
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("packet_out_decap") packet_out_decap() packet_out_decap_inst_0;
    @name("tunnel_termination_lookup") tunnel_termination_lookup() tunnel_termination_lookup_inst_0;
    @name("vlan_untag") vlan_untag() vlan_untag_inst_0;
    @name("acl_pre_ingress") acl_pre_ingress() acl_pre_ingress_inst_0;
    @name("ingress_vlan_checks") ingress_vlan_checks() ingress_vlan_checks_inst_0;
    @name("tunnel_termination_decap") tunnel_termination_decap() tunnel_termination_decap_inst_0;
    @name("admit_google_system_mac") admit_google_system_mac() admit_google_system_mac_inst_0;
    @name("l3_admit") l3_admit() l3_admit_inst_0;
    @name("hashing") hashing() hashing_inst_0;
    @name("routing_lookup") routing_lookup() routing_lookup_inst_0;
    @name("acl_ingress") acl_ingress() acl_ingress_inst_0;
    @name("routing_resolution") routing_resolution() routing_resolution_inst_0;
    @name("mirror_session_lookup") mirror_session_lookup() mirror_session_lookup_inst_0;
    @name("ingress_cloning") ingress_cloning() ingress_cloning_inst_0;
    @name("drop_martians") drop_martians() drop_martians_inst_0;
    apply {
        packet_out_decap_inst_0.apply(headers, local_metadata, standard_metadata);
        if (local_metadata.bypass_ingress) {
            ;
        } else {
            tunnel_termination_lookup_inst_0.apply(headers, local_metadata);
            vlan_untag_inst_0.apply(headers, local_metadata, standard_metadata);
            acl_pre_ingress_inst_0.apply(headers, local_metadata, standard_metadata);
            ingress_vlan_checks_inst_0.apply(headers, local_metadata, standard_metadata);
            tunnel_termination_decap_inst_0.apply(headers, local_metadata);
            admit_google_system_mac_inst_0.apply(headers, local_metadata);
            l3_admit_inst_0.apply(headers, local_metadata, standard_metadata);
            hashing_inst_0.apply(headers, local_metadata, standard_metadata);
            routing_lookup_inst_0.apply(headers, local_metadata, standard_metadata);
            acl_ingress_inst_0.apply(headers, local_metadata, standard_metadata);
            routing_resolution_inst_0.apply(headers, local_metadata, standard_metadata);
            mirror_session_lookup_inst_0.apply(headers, local_metadata, standard_metadata);
            ingress_cloning_inst_0.apply(headers, local_metadata, standard_metadata);
            drop_martians_inst_0.apply(headers, local_metadata, standard_metadata);
        }
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("packet_in_encap") packet_in_encap() packet_in_encap_inst_0;
    @name("packet_rewrites") packet_rewrites() packet_rewrites_inst_0;
    @name("gre_tunnel_encap") gre_tunnel_encap() gre_tunnel_encap_inst_0;
    @name("mirroring_encap") mirroring_encap() mirroring_encap_inst_0;
    @name("egress_vlan_checks") egress_vlan_checks() egress_vlan_checks_inst_0;
    @name("vlan_tag") vlan_tag() vlan_tag_inst_0;
    @name("acl_egress") acl_egress() acl_egress_inst_0;
    apply {
        packet_in_encap_inst_0.apply(headers, local_metadata, standard_metadata);
        if (standard_metadata.instance_type == 32w1 && standard_metadata.egress_rid == 16w1) {
            ;
        } else {
            packet_rewrites_inst_0.apply(headers, local_metadata, standard_metadata);
            gre_tunnel_encap_inst_0.apply(headers, local_metadata, standard_metadata);
            mirroring_encap_inst_0.apply(headers, local_metadata, standard_metadata);
            egress_vlan_checks_inst_0.apply(headers, local_metadata, standard_metadata);
            vlan_tag_inst_0.apply(headers, local_metadata, standard_metadata);
            acl_egress_inst_0.apply(headers, local_metadata, standard_metadata);
        }
    }
}

@pkginfo(name="middleblock.p4", organization="Google", version="1.6.1") V1Switch<headers_t, local_metadata_t>(packet_parser(), verify_ipv4_checksum(), ingress(), egress(), compute_ipv4_checksum(), packet_deparser()) main;
