error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    InvalidIPv4Header
}
#include <core.p4>
#include <pna.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<128> IPv6Address;
typedef bit<128> IPv4ORv6Address;
header ethernet_t {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    bit<16>         ether_type;
}

const bit<16> ETHER_HDR_SIZE = 112 / 8;
header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdr_checksum;
    IPv4Address src_addr;
    IPv4Address dst_addr;
}

const bit<16> IPV4_HDR_SIZE = 160 / 8;
header ipv4options_t {
    varbit<320> options;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

const bit<16> UDP_HDR_SIZE = 64 / 8;
header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

const bit<16> VXLAN_HDR_SIZE = 64 / 8;
header nvgre_t {
    bit<4>  flags;
    bit<9>  reserved;
    bit<3>  version;
    bit<16> protocol_type;
    bit<24> vsid;
    bit<8>  flow_id;
}

const bit<16> NVGRE_HDR_SIZE = 64 / 8;
header tcp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

const bit<16> TCP_HDR_SIZE = 160 / 8;
header ipv6_t {
    bit<4>      version;
    bit<8>      traffic_class;
    bit<20>     flow_label;
    bit<16>     payload_length;
    bit<8>      next_header;
    bit<8>      hop_limit;
    IPv6Address src_addr;
    IPv6Address dst_addr;
}

const bit<16> IPV6_HDR_SIZE = 320 / 8;
struct headers_t {
    ethernet_t    u1_ethernet;
    ipv4_t        u1_ipv4;
    ipv4options_t u1_ipv4options;
    ipv6_t        u1_ipv6;
    udp_t         u1_udp;
    tcp_t         u1_tcp;
    vxlan_t       u1_vxlan;
    nvgre_t       u1_nvgre;
    ethernet_t    u0_ethernet;
    ipv4_t        u0_ipv4;
    ipv4options_t u0_ipv4options;
    ipv6_t        u0_ipv6;
    udp_t         u0_udp;
    tcp_t         u0_tcp;
    vxlan_t       u0_vxlan;
    nvgre_t       u0_nvgre;
    ethernet_t    customer_ethernet;
    ipv4_t        customer_ipv4;
    ipv6_t        customer_ipv6;
    udp_t         customer_udp;
    tcp_t         customer_tcp;
}

enum bit<16> dash_encapsulation_t {
    INVALID = 0,
    VXLAN = 1,
    NVGRE = 2
}

enum bit<32> dash_routing_actions_t {
    NONE = 0,
    STATIC_ENCAP = 1 << 0,
    NAT46 = 1 << 1,
    NAT64 = 1 << 2
}

enum bit<16> dash_direction_t {
    INVALID = 0,
    OUTBOUND = 1,
    INBOUND = 2
}

enum bit<16> dash_pipeline_stage_t {
    INVALID = 0,
    INBOUND_STAGE_START = 100,
    OUTBOUND_STAGE_START = 200,
    OUTBOUND_ROUTING = 200,
    OUTBOUND_MAPPING = 201,
    ROUTING_ACTION_APPLY = 300
}

struct conntrack_data_t {
    bool allow_in;
    bool allow_out;
}

enum bit<16> dash_tunnel_dscp_mode_t {
    PRESERVE_MODEL = 0,
    PIPE_MODEL = 1
}

struct eni_data_t {
    bit<32>                 cps;
    bit<32>                 pps;
    bit<32>                 flows;
    bit<1>                  admin_state;
    IPv6Address             pl_sip;
    IPv6Address             pl_sip_mask;
    IPv4Address             pl_underlay_sip;
    bit<6>                  dscp;
    dash_tunnel_dscp_mode_t dscp_mode;
}

struct encap_data_t {
    bit<24>              vni;
    bit<24>              dest_vnet_vni;
    IPv4Address          underlay_sip;
    IPv4Address          underlay_dip;
    EthernetAddress      underlay_smac;
    EthernetAddress      underlay_dmac;
    dash_encapsulation_t dash_encapsulation;
}

struct overlay_rewrite_data_t {
    bool            is_ipv6;
    EthernetAddress dmac;
    IPv4ORv6Address sip;
    IPv4ORv6Address dip;
    IPv6Address     sip_mask;
    IPv6Address     dip_mask;
}

struct metadata_t {
    dash_direction_t       direction;
    EthernetAddress        eni_addr;
    bit<16>                vnet_id;
    bit<16>                dst_vnet_id;
    bit<16>                eni_id;
    eni_data_t             eni_data;
    bit<16>                inbound_vm_id;
    bit<8>                 appliance_id;
    bit<1>                 is_overlay_ip_v6;
    bit<1>                 is_lkup_dst_ip_v6;
    bit<8>                 ip_protocol;
    IPv4ORv6Address        dst_ip_addr;
    IPv4ORv6Address        src_ip_addr;
    IPv4ORv6Address        lkup_dst_ip_addr;
    conntrack_data_t       conntrack_data;
    bit<16>                src_l4_port;
    bit<16>                dst_l4_port;
    bit<16>                stage1_dash_acl_group_id;
    bit<16>                stage2_dash_acl_group_id;
    bit<16>                stage3_dash_acl_group_id;
    bit<16>                stage4_dash_acl_group_id;
    bit<16>                stage5_dash_acl_group_id;
    bit<1>                 meter_policy_en;
    bit<1>                 mapping_meter_class_override;
    bit<16>                meter_policy_id;
    bit<16>                policy_meter_class;
    bit<16>                route_meter_class;
    bit<16>                mapping_meter_class;
    bit<16>                meter_class;
    bit<32>                meter_bucket_index;
    bit<16>                tunnel_pointer;
    bool                   is_fast_path_icmp_flow_redirection_packet;
    bit<1>                 fast_path_icmp_flow_redirection_disabled;
    dash_pipeline_stage_t  target_stage;
    bit<32>                routing_actions;
    bool                   dropped;
    encap_data_t           encap_data;
    overlay_rewrite_data_t overlay_data;
}

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        packet.extract(hd.u0_ethernet);
        transition select(hd.u0_ethernet.ether_type) {
            0x800: parse_u0_ipv4;
            0x86dd: parse_u0_ipv6;
            default: accept;
        }
    }
    state parse_u0_ipv4 {
        packet.extract(hd.u0_ipv4);
        verify(hd.u0_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.u0_ipv4.ihl >= 5, error.InvalidIPv4Header);
        transition select(hd.u0_ipv4.ihl) {
            5: dispatch_on_u0_protocol;
            default: parse_u0_ipv4options;
        }
    }
    state parse_u0_ipv4options {
        packet.extract(hd.u0_ipv4options, (bit<32>)(((bit<16>)hd.u0_ipv4.ihl - 5) * 32));
        transition dispatch_on_u0_protocol;
    }
    state dispatch_on_u0_protocol {
        transition select(hd.u0_ipv4.protocol) {
            17: parse_u0_udp;
            6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_ipv6 {
        packet.extract(hd.u0_ipv6);
        transition select(hd.u0_ipv6.next_header) {
            17: parse_u0_udp;
            6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_udp {
        packet.extract(hd.u0_udp);
        transition select(hd.u0_udp.dst_port) {
            4789: parse_u0_vxlan;
            default: accept;
        }
    }
    state parse_u0_tcp {
        packet.extract(hd.u0_tcp);
        transition accept;
    }
    state parse_u0_vxlan {
        packet.extract(hd.u0_vxlan);
        transition parse_customer_ethernet;
    }
    state parse_customer_ethernet {
        packet.extract(hd.customer_ethernet);
        transition select(hd.customer_ethernet.ether_type) {
            0x800: parse_customer_ipv4;
            0x86dd: parse_customer_ipv6;
            default: accept;
        }
    }
    state parse_customer_ipv4 {
        packet.extract(hd.customer_ipv4);
        verify(hd.customer_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.customer_ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hd.customer_ipv4.protocol) {
            17: parse_customer_udp;
            6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_ipv6 {
        packet.extract(hd.customer_ipv6);
        transition select(hd.customer_ipv6.next_header) {
            17: parse_customer_udp;
            6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_tcp {
        packet.extract(hd.customer_tcp);
        transition accept;
    }
    state parse_customer_udp {
        packet.extract(hd.customer_udp);
        transition accept;
    }
}

control dash_deparser(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.u0_ethernet);
        packet.emit(hdr.u0_ipv4);
        packet.emit(hdr.u0_ipv4options);
        packet.emit(hdr.u0_ipv6);
        packet.emit(hdr.u0_udp);
        packet.emit(hdr.u0_tcp);
        packet.emit(hdr.u0_vxlan);
        packet.emit(hdr.u0_nvgre);
        packet.emit(hdr.customer_ethernet);
        packet.emit(hdr.customer_ipv4);
        packet.emit(hdr.customer_ipv6);
        packet.emit(hdr.customer_tcp);
        packet.emit(hdr.customer_udp);
    }
}

action push_vxlan_tunnel_u0(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.customer_ethernet.dst_addr = overlay_dmac;
    hdr.u0_ethernet.setValid();
    hdr.u0_ethernet.dst_addr = underlay_dmac;
    hdr.u0_ethernet.src_addr = underlay_smac;
    hdr.u0_ethernet.ether_type = 0x800;
    hdr.u0_ipv4.setValid();
    bit<16> customer_ip_len = 0;
    if (hdr.customer_ipv4.isValid()) {
        customer_ip_len = customer_ip_len + hdr.customer_ipv4.total_len;
    }
    if (hdr.customer_ipv6.isValid()) {
        customer_ip_len = customer_ip_len + IPV6_HDR_SIZE + hdr.customer_ipv6.payload_length;
    }
    hdr.u0_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + VXLAN_HDR_SIZE + customer_ip_len;
    hdr.u0_ipv4.version = 4;
    hdr.u0_ipv4.ihl = 5;
    hdr.u0_ipv4.diffserv = 0;
    hdr.u0_ipv4.identification = 1;
    hdr.u0_ipv4.flags = 0;
    hdr.u0_ipv4.frag_offset = 0;
    hdr.u0_ipv4.ttl = 64;
    hdr.u0_ipv4.protocol = 17;
    hdr.u0_ipv4.dst_addr = underlay_dip;
    hdr.u0_ipv4.src_addr = underlay_sip;
    hdr.u0_ipv4.hdr_checksum = 0;
    hdr.u0_udp.setValid();
    hdr.u0_udp.src_port = 0;
    hdr.u0_udp.dst_port = 4789;
    hdr.u0_udp.length = UDP_HDR_SIZE + VXLAN_HDR_SIZE + ETHER_HDR_SIZE + customer_ip_len;
    hdr.u0_udp.checksum = 0;
    hdr.u0_vxlan.setValid();
    hdr.u0_vxlan.reserved = 0;
    hdr.u0_vxlan.reserved_2 = 0;
    hdr.u0_vxlan.flags = 0;
    hdr.u0_vxlan.vni = tunnel_key;
}
action push_vxlan_tunnel_u1(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.u0_ethernet.dst_addr = overlay_dmac;
    hdr.u1_ethernet.setValid();
    hdr.u1_ethernet.dst_addr = underlay_dmac;
    hdr.u1_ethernet.src_addr = underlay_smac;
    hdr.u1_ethernet.ether_type = 0x800;
    hdr.u1_ipv4.setValid();
    bit<16> u0_ip_len = 0;
    if (hdr.u0_ipv4.isValid()) {
        u0_ip_len = u0_ip_len + hdr.u0_ipv4.total_len;
    }
    if (hdr.u0_ipv6.isValid()) {
        u0_ip_len = u0_ip_len + IPV6_HDR_SIZE + hdr.u0_ipv6.payload_length;
    }
    hdr.u1_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + VXLAN_HDR_SIZE + u0_ip_len;
    hdr.u1_ipv4.version = 4;
    hdr.u1_ipv4.ihl = 5;
    hdr.u1_ipv4.diffserv = 0;
    hdr.u1_ipv4.identification = 1;
    hdr.u1_ipv4.flags = 0;
    hdr.u1_ipv4.frag_offset = 0;
    hdr.u1_ipv4.ttl = 64;
    hdr.u1_ipv4.protocol = 17;
    hdr.u1_ipv4.dst_addr = underlay_dip;
    hdr.u1_ipv4.src_addr = underlay_sip;
    hdr.u1_ipv4.hdr_checksum = 0;
    hdr.u1_udp.setValid();
    hdr.u1_udp.src_port = 0;
    hdr.u1_udp.dst_port = 4789;
    hdr.u1_udp.length = UDP_HDR_SIZE + VXLAN_HDR_SIZE + ETHER_HDR_SIZE + u0_ip_len;
    hdr.u1_udp.checksum = 0;
    hdr.u1_vxlan.setValid();
    hdr.u1_vxlan.reserved = 0;
    hdr.u1_vxlan.reserved_2 = 0;
    hdr.u1_vxlan.flags = 0;
    hdr.u1_vxlan.vni = tunnel_key;
}
action push_nvgre_tunnel_u0(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.customer_ethernet.dst_addr = overlay_dmac;
    hdr.u0_ethernet.setValid();
    hdr.u0_ethernet.dst_addr = underlay_dmac;
    hdr.u0_ethernet.src_addr = underlay_smac;
    hdr.u0_ethernet.ether_type = 0x800;
    hdr.u0_ipv4.setValid();
    bit<16> customer_ip_len = 0;
    if (hdr.customer_ipv4.isValid()) {
        customer_ip_len = customer_ip_len + hdr.customer_ipv4.total_len;
    }
    if (hdr.customer_ipv6.isValid()) {
        customer_ip_len = customer_ip_len + IPV6_HDR_SIZE + hdr.customer_ipv6.payload_length;
    }
    hdr.u0_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + NVGRE_HDR_SIZE + customer_ip_len;
    hdr.u0_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + NVGRE_HDR_SIZE + hdr.u0_ipv4.total_len;
    hdr.u0_ipv4.version = 4;
    hdr.u0_ipv4.ihl = 5;
    hdr.u0_ipv4.diffserv = 0;
    hdr.u0_ipv4.identification = 1;
    hdr.u0_ipv4.flags = 0;
    hdr.u0_ipv4.frag_offset = 0;
    hdr.u0_ipv4.ttl = 64;
    hdr.u0_ipv4.protocol = 0x2f;
    hdr.u0_ipv4.dst_addr = underlay_dip;
    hdr.u0_ipv4.src_addr = underlay_sip;
    hdr.u0_ipv4.hdr_checksum = 0;
    hdr.u0_nvgre.setValid();
    hdr.u0_nvgre.flags = 4;
    hdr.u0_nvgre.reserved = 0;
    hdr.u0_nvgre.version = 0;
    hdr.u0_nvgre.protocol_type = 0x6558;
    hdr.u0_nvgre.vsid = tunnel_key;
    hdr.u0_nvgre.flow_id = 0;
}
action push_nvgre_tunnel_u1(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.u0_ethernet.dst_addr = overlay_dmac;
    hdr.u1_ethernet.setValid();
    hdr.u1_ethernet.dst_addr = underlay_dmac;
    hdr.u1_ethernet.src_addr = underlay_smac;
    hdr.u1_ethernet.ether_type = 0x800;
    hdr.u1_ipv4.setValid();
    bit<16> u0_ip_len = 0;
    if (hdr.u0_ipv4.isValid()) {
        u0_ip_len = u0_ip_len + hdr.u0_ipv4.total_len;
    }
    if (hdr.u0_ipv6.isValid()) {
        u0_ip_len = u0_ip_len + IPV6_HDR_SIZE + hdr.u0_ipv6.payload_length;
    }
    hdr.u1_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + NVGRE_HDR_SIZE + u0_ip_len;
    hdr.u1_ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + NVGRE_HDR_SIZE + hdr.u1_ipv4.total_len;
    hdr.u1_ipv4.version = 4;
    hdr.u1_ipv4.ihl = 5;
    hdr.u1_ipv4.diffserv = 0;
    hdr.u1_ipv4.identification = 1;
    hdr.u1_ipv4.flags = 0;
    hdr.u1_ipv4.frag_offset = 0;
    hdr.u1_ipv4.ttl = 64;
    hdr.u1_ipv4.protocol = 0x2f;
    hdr.u1_ipv4.dst_addr = underlay_dip;
    hdr.u1_ipv4.src_addr = underlay_sip;
    hdr.u1_ipv4.hdr_checksum = 0;
    hdr.u1_nvgre.setValid();
    hdr.u1_nvgre.flags = 4;
    hdr.u1_nvgre.reserved = 0;
    hdr.u1_nvgre.version = 0;
    hdr.u1_nvgre.protocol_type = 0x6558;
    hdr.u1_nvgre.vsid = tunnel_key;
    hdr.u1_nvgre.flow_id = 0;
}
action tunnel_decap(inout headers_t hdr, inout metadata_t meta) {
    hdr.u0_ethernet.setInvalid();
    hdr.u0_ipv4.setInvalid();
    hdr.u0_ipv6.setInvalid();
    hdr.u0_nvgre.setInvalid();
    hdr.u0_vxlan.setInvalid();
    hdr.u0_udp.setInvalid();
    meta.tunnel_pointer = 0;
}
match_kind {
    list,
    range_list
}

control acl(inout headers_t hdr, inout metadata_t meta) {
    action permit() {
    }
    action permit_and_continue() {
    }
    action deny() {
        meta.dropped = true;
    }
    action deny_and_continue() {
        meta.dropped = true;
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", isobject="true"] table stage1 {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"];
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"];
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"];
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"];
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"];
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"];
        }
        actions = {
            permit;
            permit_and_continue;
            deny;
            deny_and_continue;
        }
        default_action = deny;
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", isobject="true"] table stage2 {
        key = {
            meta.stage2_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"];
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"];
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"];
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"];
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"];
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"];
        }
        actions = {
            permit;
            permit_and_continue;
            deny;
            deny_and_continue;
        }
        default_action = deny;
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", isobject="true"] table stage3 {
        key = {
            meta.stage3_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"];
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"];
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"];
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"];
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"];
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"];
        }
        actions = {
            permit;
            permit_and_continue;
            deny;
            deny_and_continue;
        }
        default_action = deny;
    }
    apply {
        if (meta.stage1_dash_acl_group_id != 0) {
            switch (stage1.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
            }
        }
        if (meta.stage2_dash_acl_group_id != 0) {
            switch (stage2.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
            }
        }
        if (meta.stage3_dash_acl_group_id != 0) {
            switch (stage3.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
            }
        }
    }
}

action push_action_static_encap(in headers_t hdr, inout metadata_t meta, in dash_encapsulation_t encap=dash_encapsulation_t.VXLAN, in bit<24> vni=0, in IPv4Address underlay_sip=0, in IPv4Address underlay_dip=0, in EthernetAddress underlay_smac=0, in EthernetAddress underlay_dmac=0, in EthernetAddress overlay_dmac=0) {
    meta.routing_actions = meta.routing_actions | dash_routing_actions_t.STATIC_ENCAP;
    meta.encap_data.dash_encapsulation = encap;
    meta.encap_data.vni = (vni == 0 ? meta.encap_data.vni : vni);
    meta.encap_data.underlay_smac = (underlay_smac == 0 ? meta.encap_data.underlay_smac : underlay_smac);
    meta.encap_data.underlay_dmac = (underlay_dmac == 0 ? meta.encap_data.underlay_dmac : underlay_dmac);
    meta.encap_data.underlay_sip = (underlay_sip == 0 ? meta.encap_data.underlay_sip : underlay_sip);
    meta.encap_data.underlay_dip = (underlay_dip == 0 ? meta.encap_data.underlay_dip : underlay_dip);
    meta.overlay_data.dmac = (overlay_dmac == 0 ? meta.overlay_data.dmac : overlay_dmac);
}
control do_action_static_encap(inout headers_t hdr, inout metadata_t meta) {
    apply {
        if (meta.routing_actions & dash_routing_actions_t.STATIC_ENCAP == 0) {
            return;
        }
        if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
            if (meta.tunnel_pointer == 0) {
                push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            } else if (meta.tunnel_pointer == 1) {
                push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            }
        } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
            if (meta.tunnel_pointer == 0) {
                push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            } else if (meta.tunnel_pointer == 1) {
                push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            }
        }
        meta.tunnel_pointer = meta.tunnel_pointer + 1;
    }
}

action push_action_nat46(in headers_t hdr, inout metadata_t meta, in IPv6Address sip, in IPv6Address sip_mask, in IPv6Address dip, in IPv6Address dip_mask) {
    meta.routing_actions = meta.routing_actions | dash_routing_actions_t.NAT46;
    meta.overlay_data.is_ipv6 = true;
    meta.overlay_data.sip = sip;
    meta.overlay_data.sip_mask = sip_mask;
    meta.overlay_data.dip = dip;
    meta.overlay_data.dip_mask = dip_mask;
}
control do_action_nat46(inout headers_t hdr, in metadata_t meta) {
    apply {
        if (meta.routing_actions & dash_routing_actions_t.NAT46 == 0) {
            return;
        }
        ;
        hdr.u0_ipv6.setValid();
        hdr.u0_ipv6.version = 6;
        hdr.u0_ipv6.traffic_class = 0;
        hdr.u0_ipv6.flow_label = 0;
        hdr.u0_ipv6.payload_length = hdr.u0_ipv4.total_len - IPV4_HDR_SIZE;
        hdr.u0_ipv6.next_header = hdr.u0_ipv4.protocol;
        hdr.u0_ipv6.hop_limit = hdr.u0_ipv4.ttl;
        hdr.u0_ipv6.dst_addr = (IPv6Address)hdr.u0_ipv4.dst_addr & ~meta.overlay_data.dip_mask | meta.overlay_data.dip & meta.overlay_data.dip_mask;
        hdr.u0_ipv6.src_addr = (IPv6Address)hdr.u0_ipv4.src_addr & ~meta.overlay_data.sip_mask | meta.overlay_data.sip & meta.overlay_data.sip_mask;
        hdr.u0_ipv4.setInvalid();
        hdr.u0_ethernet.ether_type = 0x86dd;
    }
}

action push_action_nat64(in headers_t hdr, inout metadata_t meta, in IPv4Address src, in IPv4Address dst) {
    meta.routing_actions = meta.routing_actions | dash_routing_actions_t.NAT64;
    meta.overlay_data.is_ipv6 = false;
    meta.overlay_data.sip = (IPv4ORv6Address)src;
    meta.overlay_data.dip = (IPv4ORv6Address)dst;
}
control do_action_nat64(inout headers_t hdr, in metadata_t meta) {
    apply {
        if (meta.routing_actions & dash_routing_actions_t.NAT64 == 0) {
            return;
        }
        ;
        hdr.u0_ipv4.setValid();
        hdr.u0_ipv4.version = 4;
        hdr.u0_ipv4.ihl = 5;
        hdr.u0_ipv4.diffserv = 0;
        hdr.u0_ipv4.total_len = hdr.u0_ipv6.payload_length + IPV4_HDR_SIZE;
        hdr.u0_ipv4.identification = 1;
        hdr.u0_ipv4.flags = 0;
        hdr.u0_ipv4.frag_offset = 0;
        hdr.u0_ipv4.protocol = hdr.u0_ipv6.next_header;
        hdr.u0_ipv4.ttl = hdr.u0_ipv6.hop_limit;
        hdr.u0_ipv4.hdr_checksum = 0;
        hdr.u0_ipv4.dst_addr = (IPv4Address)meta.overlay_data.dip;
        hdr.u0_ipv4.src_addr = (IPv4Address)meta.overlay_data.sip;
        hdr.u0_ipv6.setInvalid();
        hdr.u0_ethernet.ether_type = 0x800;
    }
}

action set_route_meter_attrs(inout metadata_t meta, bit<1> meter_policy_en, bit<16> meter_class) {
    meta.meter_policy_en = meter_policy_en;
    meta.route_meter_class = meter_class;
}
action set_mapping_meter_attr(inout metadata_t meta, in bit<16> meter_class, in bit<1> meter_class_override) {
    meta.mapping_meter_class = meter_class;
    meta.mapping_meter_class_override = meter_class_override;
}
action drop(inout metadata_t meta) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    meta.dropped = true;
}
action route_vnet(inout headers_t hdr, inout metadata_t meta, @SaiVal[type="sai_object_id_t"] bit<16> dst_vnet_id, bit<1> meter_policy_en, bit<16> meter_class) {
    meta.target_stage = dash_pipeline_stage_t.OUTBOUND_MAPPING;
    meta.dst_vnet_id = dst_vnet_id;
    set_route_meter_attrs(meta, meter_policy_en, meter_class);
}
action route_vnet_direct(inout headers_t hdr, inout metadata_t meta, bit<16> dst_vnet_id, bit<1> overlay_ip_is_v6, @SaiVal[type="sai_ip_address_t"] IPv4ORv6Address overlay_ip, bit<1> meter_policy_en, bit<16> meter_class) {
    meta.target_stage = dash_pipeline_stage_t.OUTBOUND_MAPPING;
    meta.dst_vnet_id = dst_vnet_id;
    meta.lkup_dst_ip_addr = overlay_ip;
    meta.is_lkup_dst_ip_v6 = overlay_ip_is_v6;
    set_route_meter_attrs(meta, meter_policy_en, meter_class);
}
action route_direct(inout headers_t hdr, inout metadata_t meta, bit<1> meter_policy_en, bit<16> meter_class) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    set_route_meter_attrs(meta, meter_policy_en, meter_class);
}
action route_service_tunnel(inout headers_t hdr, inout metadata_t meta, bit<1> overlay_dip_is_v6, IPv4ORv6Address overlay_dip, bit<1> overlay_dip_mask_is_v6, IPv4ORv6Address overlay_dip_mask, bit<1> overlay_sip_is_v6, IPv4ORv6Address overlay_sip, bit<1> overlay_sip_mask_is_v6, IPv4ORv6Address overlay_sip_mask, bit<1> underlay_dip_is_v6, IPv4ORv6Address underlay_dip, bit<1> underlay_sip_is_v6, IPv4ORv6Address underlay_sip, @SaiVal[type="sai_dash_encapsulation_t", default_value="SAI_DASH_ENCAPSULATION_VXLAN"] dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<1> meter_policy_en, bit<16> meter_class) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    push_action_nat46(hdr = hdr, meta = meta, sip = overlay_sip, sip_mask = overlay_sip_mask, dip = overlay_dip, dip_mask = overlay_dip_mask);
    push_action_static_encap(hdr = hdr, meta = meta, encap = dash_encapsulation, vni = tunnel_key, underlay_sip = (underlay_sip == 0 ? hdr.u0_ipv4.src_addr : (IPv4Address)underlay_sip), underlay_dip = (underlay_dip == 0 ? hdr.u0_ipv4.dst_addr : (IPv4Address)underlay_dip), overlay_dmac = hdr.u0_ethernet.dst_addr);
    set_route_meter_attrs(meta, meter_policy_en, meter_class);
}
action set_tunnel_mapping(inout headers_t hdr, inout metadata_t meta, @SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, EthernetAddress overlay_dmac, bit<1> use_dst_vnet_vni, bit<16> meter_class, bit<1> meter_class_override) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    if (use_dst_vnet_vni == 1) {
        meta.vnet_id = meta.dst_vnet_id;
    }
    push_action_static_encap(hdr = hdr, meta = meta, underlay_dip = underlay_dip, overlay_dmac = overlay_dmac);
    set_mapping_meter_attr(meta, meter_class, meter_class_override);
}
action set_private_link_mapping(inout headers_t hdr, inout metadata_t meta, @SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, IPv6Address overlay_sip, IPv6Address overlay_dip, @SaiVal[type="sai_dash_encapsulation_t"] dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<16> meter_class, bit<1> meter_class_override) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    push_action_static_encap(hdr = hdr, meta = meta, encap = dash_encapsulation, vni = tunnel_key, underlay_sip = meta.eni_data.pl_underlay_sip, underlay_dip = underlay_dip, overlay_dmac = hdr.u0_ethernet.dst_addr);
    push_action_nat46(hdr = hdr, meta = meta, dip = overlay_dip, dip_mask = 0xffffffffffffffffffffffff, sip = overlay_sip & ~meta.eni_data.pl_sip_mask | meta.eni_data.pl_sip | (IPv6Address)hdr.u0_ipv4.src_addr, sip_mask = 0xffffffffffffffffffffffff);
    set_mapping_meter_attr(meta, meter_class, meter_class_override);
}
control outbound_routing_stage(inout headers_t hdr, inout metadata_t meta) {
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] table routing {
        key = {
            meta.eni_id          : exact @SaiVal[type="sai_object_id_t"];
            meta.is_overlay_ip_v6: exact @SaiVal[name="destination_is_v6"];
            meta.dst_ip_addr     : lpm @SaiVal[name="destination"];
        }
        actions = {
            route_vnet(hdr, meta);
            route_vnet_direct(hdr, meta);
            route_direct(hdr, meta);
            route_service_tunnel(hdr, meta);
            drop(meta);
        }
        const default_action = drop(meta);
    }
    apply {
        if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_ROUTING) {
            return;
        }
        routing.apply();
    }
}

control outbound_mapping_stage(inout headers_t hdr, inout metadata_t meta) {
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] table ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @SaiVal[type="sai_object_id_t"];
            meta.is_lkup_dst_ip_v6: exact @SaiVal[name="dip_is_v6"];
            meta.lkup_dst_ip_addr : exact @SaiVal[name="dip"];
        }
        actions = {
            set_tunnel_mapping(hdr, meta);
            set_private_link_mapping(hdr, meta);
            @defaultonly drop(meta);
        }
        const default_action = drop(meta);
    }
    action set_vnet_attrs(bit<24> vni) {
        meta.encap_data.vni = vni;
    }
    @SaiTable[name="vnet", api="dash_vnet", isobject="true"] table vnet {
        key = {
            meta.vnet_id: exact @SaiVal[type="sai_object_id_t"];
        }
        actions = {
            set_vnet_attrs;
        }
    }
    apply {
        if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_MAPPING) {
            return;
        }
        switch (ca_to_pa.apply().action_run) {
            set_tunnel_mapping: {
                vnet.apply();
            }
        }
    }
}

control outbound(inout headers_t hdr, inout metadata_t meta) {
    apply {
        if (!meta.conntrack_data.allow_out) {
            acl.apply(hdr, meta);
        }
        meta.lkup_dst_ip_addr = meta.dst_ip_addr;
        meta.is_lkup_dst_ip_v6 = meta.is_overlay_ip_v6;
        outbound_routing_stage.apply(hdr, meta);
        outbound_mapping_stage.apply(hdr, meta);
    }
}

action service_tunnel_encode(inout headers_t hdr, in IPv6Address st_dst, in IPv6Address st_dst_mask, in IPv6Address st_src, in IPv6Address st_src_mask) {
    hdr.u0_ipv6.setValid();
    hdr.u0_ipv6.version = 6;
    hdr.u0_ipv6.traffic_class = 0;
    hdr.u0_ipv6.flow_label = 0;
    hdr.u0_ipv6.payload_length = hdr.u0_ipv4.total_len - IPV4_HDR_SIZE;
    hdr.u0_ipv6.next_header = hdr.u0_ipv4.protocol;
    hdr.u0_ipv6.hop_limit = hdr.u0_ipv4.ttl;
    hdr.u0_ipv6.dst_addr = (IPv6Address)hdr.u0_ipv4.dst_addr & ~st_dst_mask | st_dst & st_dst_mask;
    hdr.u0_ipv6.src_addr = (IPv6Address)hdr.u0_ipv4.src_addr & ~st_src_mask | st_src & st_src_mask;
    hdr.u0_ipv4.setInvalid();
    hdr.u0_ethernet.ether_type = 0x86dd;
}
action service_tunnel_decode(inout headers_t hdr, in IPv4Address src, in IPv4Address dst) {
    hdr.u0_ipv4.setValid();
    hdr.u0_ipv4.version = 4;
    hdr.u0_ipv4.ihl = 5;
    hdr.u0_ipv4.diffserv = 0;
    hdr.u0_ipv4.total_len = hdr.u0_ipv6.payload_length + IPV4_HDR_SIZE;
    hdr.u0_ipv4.identification = 1;
    hdr.u0_ipv4.flags = 0;
    hdr.u0_ipv4.frag_offset = 0;
    hdr.u0_ipv4.protocol = hdr.u0_ipv6.next_header;
    hdr.u0_ipv4.ttl = hdr.u0_ipv6.hop_limit;
    hdr.u0_ipv4.hdr_checksum = 0;
    hdr.u0_ipv4.dst_addr = dst;
    hdr.u0_ipv4.src_addr = src;
    hdr.u0_ipv6.setInvalid();
    hdr.u0_ethernet.ether_type = 0x800;
}
control inbound(inout headers_t hdr, inout metadata_t meta) {
    apply {
        if (!meta.conntrack_data.allow_in) {
            acl.apply(hdr, meta);
        }
        {
            if (dash_encapsulation_t.VXLAN == dash_encapsulation_t.VXLAN) {
                if (meta.tunnel_pointer == 0) {
                    push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
                } else if (meta.tunnel_pointer == 1) {
                    push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
                }
            } else if (dash_encapsulation_t.VXLAN == dash_encapsulation_t.NVGRE) {
                if (meta.tunnel_pointer == 0) {
                    push_nvgre_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
                } else if (meta.tunnel_pointer == 1) {
                    push_nvgre_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
                }
            }
            meta.tunnel_pointer = meta.tunnel_pointer + 1;
        }
        ;
    }
}

control direction_lookup_stage(inout headers_t hdr, inout metadata_t meta) {
    action set_outbound_direction() {
        meta.direction = dash_direction_t.OUTBOUND;
    }
    action set_inbound_direction() {
        meta.direction = dash_direction_t.INBOUND;
    }
    @SaiTable[name="direction_lookup", api="dash_direction_lookup"] table direction_lookup {
        key = {
            hdr.u0_vxlan.vni: exact @SaiVal[name="VNI"];
        }
        actions = {
            set_outbound_direction;
            @defaultonly set_inbound_direction;
        }
        const default_action = set_inbound_direction;
    }
    apply {
        direction_lookup.apply();
    }
}

control eni_lookup_stage(inout headers_t hdr, inout metadata_t meta) {
    action set_eni(@SaiVal[type="sai_object_id_t"] bit<16> eni_id) {
        meta.eni_id = eni_id;
    }
    action deny() {
        meta.dropped = true;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", order=0] table eni_ether_address_map {
        key = {
            meta.eni_addr: exact @SaiVal[name="address", type="sai_mac_t"];
        }
        actions = {
            set_eni;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    apply {
        meta.eni_addr = (meta.direction == dash_direction_t.OUTBOUND ? hdr.customer_ethernet.src_addr : hdr.customer_ethernet.dst_addr);
        if (!eni_ether_address_map.apply().hit) {
            if (meta.is_fast_path_icmp_flow_redirection_packet) {
                ;
            }
        }
    }
}

control routing_action_apply(inout headers_t hdr, inout metadata_t meta) {
    apply {
        do_action_nat46.apply(hdr, meta);
        do_action_nat64.apply(hdr, meta);
        do_action_static_encap.apply(hdr, meta);
    }
}

control metering_update_stage(inout headers_t hdr, inout metadata_t meta) {
    action check_ip_addr_family(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
        if (ip_addr_family == 0) {
            if (meta.is_overlay_ip_v6 == 1) {
                meta.dropped = true;
            }
        } else {
            if (meta.is_overlay_ip_v6 == 0) {
                meta.dropped = true;
            }
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", order=1, isobject="true"] table meter_policy {
        key = {
            meta.meter_policy_id: exact;
        }
        actions = {
            check_ip_addr_family;
        }
    }
    action set_policy_meter_class(bit<16> meter_class) {
        meta.policy_meter_class = meter_class;
    }
    @SaiTable[name="meter_rule", api="dash_meter", order=2, isobject="true"] table meter_rule {
        key = {
            meta.meter_policy_id: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"];
            hdr.u0_ipv4.dst_addr: ternary @SaiVal[name="dip", type="sai_ip_address_t"];
        }
        actions = {
            set_policy_meter_class;
            @defaultonly NoAction;
        }
        const default_action = NoAction();
    }
    action meter_bucket_action(@SaiVal[type="sai_uint32_t", skipattr="true"] bit<32> meter_bucket_index) {
        meta.meter_bucket_index = meter_bucket_index;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", order=0, isobject="true"] table meter_bucket {
        key = {
            meta.eni_id     : exact @SaiVal[type="sai_object_id_t"];
            meta.meter_class: exact;
        }
        actions = {
            meter_bucket_action;
            @defaultonly NoAction;
        }
        const default_action = NoAction();
    }
    @SaiTable[ignored="true"] table eni_meter {
        key = {
            meta.eni_id   : exact @SaiVal[type="sai_object_id_t"];
            meta.direction: exact;
            meta.dropped  : exact;
        }
        actions = {
            NoAction;
        }
    }
    apply {
        if (meta.meter_policy_en == 1) {
            meter_policy.apply();
            meter_rule.apply();
        }
        {
            if (meta.meter_policy_en == 1) {
                meta.meter_class = meta.policy_meter_class;
            } else {
                meta.meter_class = meta.route_meter_class;
            }
            if (meta.meter_class == 0 || meta.mapping_meter_class_override == 1) {
                meta.meter_class = meta.mapping_meter_class;
            }
        }
        meter_bucket.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            ;
        } else if (meta.direction == dash_direction_t.INBOUND) {
            ;
        }
        eni_meter.apply();
    }
}

control underlay(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd) {
    action set_nhop(bit<9> next_hop_id) {
    }
    action pkt_act(bit<9> packet_action, bit<9> next_hop_id) {
        if (packet_action == 0) {
            meta.dropped = true;
        } else if (packet_action == 1) {
            set_nhop(next_hop_id);
        }
    }
    action def_act() {
    }
    @SaiTable[name="route", api="route", api_type="underlay"] table underlay_routing {
        key = {
            meta.dst_ip_addr: lpm @SaiVal[name="destination"];
        }
        actions = {
            pkt_act;
            @defaultonly def_act;
        }
    }
    apply {
        underlay_routing.apply();
    }
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    action drop_action() {
        drop_packet();
    }
    action deny() {
        meta.dropped = true;
    }
    action accept() {
    }
    @SaiTable[name="vip", api="dash_vip"] table vip {
        key = {
            hdr.u0_ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"];
        }
        actions = {
            accept;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action set_appliance(EthernetAddress neighbor_mac, EthernetAddress mac) {
        meta.encap_data.underlay_dmac = neighbor_mac;
        meta.encap_data.underlay_smac = mac;
    }
    @SaiTable[ignored="true"] table appliance {
        key = {
            meta.appliance_id: ternary;
        }
        actions = {
            set_appliance;
        }
    }
    action set_eni_attrs(bit<32> cps, bit<32> pps, bit<32> flows, bit<1> admin_state, @SaiVal[type="sai_ip_address_t"] IPv4Address vm_underlay_dip, @SaiVal[type="sai_uint32_t"] bit<24> vm_vni, @SaiVal[type="sai_object_id_t"] bit<16> vnet_id, IPv6Address pl_sip, IPv6Address pl_sip_mask, @SaiVal[type="sai_ip_address_t"] IPv4Address pl_underlay_sip, @SaiVal[type="sai_object_id_t"] bit<16> v4_meter_policy_id, @SaiVal[type="sai_object_id_t"] bit<16> v6_meter_policy_id, @SaiVal[type="sai_dash_tunnel_dscp_mode_t"] dash_tunnel_dscp_mode_t dash_tunnel_dscp_mode, @SaiVal[type="sai_uint8_t", validonly="SAI_ENI_ATTR_DASH_TUNNEL_DSCP_MODE == SAI_DASH_TUNNEL_DSCP_MODE_PIPE_MODEL"] bit<6> dscp, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage5_dash_acl_group_id, bit<1> disable_fast_path_icmp_flow_redirection) {
        meta.eni_data.cps = cps;
        meta.eni_data.pps = pps;
        meta.eni_data.flows = flows;
        meta.eni_data.admin_state = admin_state;
        meta.eni_data.pl_sip = pl_sip;
        meta.eni_data.pl_sip_mask = pl_sip_mask;
        meta.eni_data.pl_underlay_sip = pl_underlay_sip;
        meta.encap_data.underlay_dip = vm_underlay_dip;
        if (dash_tunnel_dscp_mode == dash_tunnel_dscp_mode_t.PIPE_MODEL) {
            meta.eni_data.dscp = dscp;
        }
        meta.encap_data.vni = vm_vni;
        meta.vnet_id = vnet_id;
        if (meta.is_overlay_ip_v6 == 1) {
            if (meta.direction == dash_direction_t.OUTBOUND) {
                meta.stage1_dash_acl_group_id = outbound_v6_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = outbound_v6_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = outbound_v6_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = outbound_v6_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = outbound_v6_stage5_dash_acl_group_id;
                ;
            } else {
                meta.stage1_dash_acl_group_id = inbound_v6_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = inbound_v6_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = inbound_v6_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = inbound_v6_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = inbound_v6_stage5_dash_acl_group_id;
                ;
            }
            meta.meter_policy_id = v6_meter_policy_id;
        } else {
            if (meta.direction == dash_direction_t.OUTBOUND) {
                meta.stage1_dash_acl_group_id = outbound_v4_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = outbound_v4_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = outbound_v4_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = outbound_v4_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = outbound_v4_stage5_dash_acl_group_id;
                ;
            } else {
                meta.stage1_dash_acl_group_id = inbound_v4_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = inbound_v4_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = inbound_v4_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = inbound_v4_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = inbound_v4_stage5_dash_acl_group_id;
                ;
            }
            meta.meter_policy_id = v4_meter_policy_id;
        }
        meta.fast_path_icmp_flow_redirection_disabled = disable_fast_path_icmp_flow_redirection;
    }
    @SaiTable[name="eni", api="dash_eni", order=1, isobject="true"] table eni {
        key = {
            meta.eni_id: exact @SaiVal[type="sai_object_id_t"];
        }
        actions = {
            set_eni_attrs;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action permit() {
    }
    action tunnel_decap_pa_validate(@SaiVal[type="sai_object_id_t"] bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] table pa_validation {
        key = {
            meta.vnet_id        : exact @SaiVal[type="sai_object_id_t"];
            hdr.u0_ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"];
        }
        actions = {
            permit;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] table inbound_routing {
        key = {
            meta.eni_id         : exact @SaiVal[type="sai_object_id_t"];
            hdr.u0_vxlan.vni    : exact @SaiVal[name="VNI"];
            hdr.u0_ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"];
        }
        actions = {
            tunnel_decap(hdr, meta);
            tunnel_decap_pa_validate;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action set_acl_group_attrs(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
        if (ip_addr_family == 0) {
            if (meta.is_overlay_ip_v6 == 1) {
                meta.dropped = true;
            }
        } else {
            if (meta.is_overlay_ip_v6 == 0) {
                meta.dropped = true;
            }
        }
    }
    @SaiTable[name="dash_acl_group", api="dash_acl", order=0, isobject="true"] table acl_group {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"];
        }
        actions = {
            set_acl_group_attrs();
        }
    }
    apply {
        if (meta.is_fast_path_icmp_flow_redirection_packet) {
            ;
        }
        if (vip.apply().hit) {
            meta.encap_data.underlay_sip = hdr.u0_ipv4.dst_addr;
        } else {
            if (meta.is_fast_path_icmp_flow_redirection_packet) {
            }
        }
        direction_lookup_stage.apply(hdr, meta);
        appliance.apply();
        eni_lookup_stage.apply(hdr, meta);
        meta.eni_data.dscp_mode = dash_tunnel_dscp_mode_t.PRESERVE_MODEL;
        meta.eni_data.dscp = (bit<6>)hdr.u0_ipv4.diffserv;
        if (meta.direction == dash_direction_t.OUTBOUND) {
            tunnel_decap(hdr, meta);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            switch (inbound_routing.apply().action_run) {
                tunnel_decap_pa_validate: {
                    pa_validation.apply();
                    tunnel_decap(hdr, meta);
                }
            }
        }
        meta.is_overlay_ip_v6 = 0;
        meta.ip_protocol = 0;
        meta.dst_ip_addr = 0;
        meta.src_ip_addr = 0;
        if (hdr.customer_ipv6.isValid()) {
            meta.ip_protocol = hdr.customer_ipv6.next_header;
            meta.src_ip_addr = hdr.customer_ipv6.src_addr;
            meta.dst_ip_addr = hdr.customer_ipv6.dst_addr;
            meta.is_overlay_ip_v6 = 1;
        } else if (hdr.customer_ipv4.isValid()) {
            meta.ip_protocol = hdr.customer_ipv4.protocol;
            meta.src_ip_addr = (bit<128>)hdr.customer_ipv4.src_addr;
            meta.dst_ip_addr = (bit<128>)hdr.customer_ipv4.dst_addr;
        }
        if (hdr.customer_tcp.isValid()) {
            meta.src_l4_port = hdr.customer_tcp.src_port;
            meta.dst_l4_port = hdr.customer_tcp.dst_port;
        } else if (hdr.customer_udp.isValid()) {
            meta.src_l4_port = hdr.customer_udp.src_port;
            meta.dst_l4_port = hdr.customer_udp.dst_port;
        }
        eni.apply();
        if (meta.eni_data.admin_state == 0) {
            deny();
        }
        if (meta.is_fast_path_icmp_flow_redirection_packet) {
            ;
        }
        acl_group.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            meta.target_stage = dash_pipeline_stage_t.OUTBOUND_ROUTING;
            outbound.apply(hdr, meta);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            inbound.apply(hdr, meta);
        }
        routing_action_apply.apply(hdr, meta);
        meta.dst_ip_addr = (bit<128>)hdr.u0_ipv4.dst_addr;
        underlay.apply(hdr, meta, istd);
        if (meta.eni_data.dscp_mode == dash_tunnel_dscp_mode_t.PIPE_MODEL) {
            hdr.u0_ipv4.diffserv = (bit<8>)meta.eni_data.dscp;
        }
        metering_update_stage.apply(hdr, meta);
        if (meta.dropped) {
            drop_action();
        }
    }
}

control dash_precontrol(in headers_t pre_hdr, inout metadata_t pre_user_meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC(dash_parser(), dash_precontrol(), dash_ingress(), dash_deparser()) main;
