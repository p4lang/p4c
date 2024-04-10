error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    InvalidIPv4Header
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
typedef bit<128> IPv6Address;
typedef bit<128> IPv4ORv6Address;
header ethernet_t {
    EthernetAddress dst_addr;
    EthernetAddress src_addr;
    bit<16>         ether_type;
}

const bit<16> ETHER_HDR_SIZE = 16w14;
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

const bit<16> IPV4_HDR_SIZE = 16w20;
header ipv4options_t {
    varbit<320> options;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

const bit<16> UDP_HDR_SIZE = 16w8;
header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

const bit<16> VXLAN_HDR_SIZE = 16w8;
header nvgre_t {
    bit<4>  flags;
    bit<9>  reserved;
    bit<3>  version;
    bit<16> protocol_type;
    bit<24> vsid;
    bit<8>  flow_id;
}

const bit<16> NVGRE_HDR_SIZE = 16w8;
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

const bit<16> TCP_HDR_SIZE = 16w20;
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

const bit<16> IPV6_HDR_SIZE = 16w40;
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
    INVALID = 16w0,
    VXLAN = 16w1,
    NVGRE = 16w2
}

enum bit<32> dash_routing_actions_t {
    NONE = 32w0,
    STATIC_ENCAP = 32w1,
    NAT46 = 32w2,
    NAT64 = 32w4
}

enum bit<16> dash_direction_t {
    INVALID = 16w0,
    OUTBOUND = 16w1,
    INBOUND = 16w2
}

enum bit<16> dash_pipeline_stage_t {
    INVALID = 16w0,
    INBOUND_STAGE_START = 16w100,
    OUTBOUND_STAGE_START = 16w200,
    OUTBOUND_ROUTING = 16w200,
    OUTBOUND_MAPPING = 16w201,
    ROUTING_ACTION_APPLY = 16w300
}

struct conntrack_data_t {
    bool allow_in;
    bool allow_out;
}

enum bit<16> dash_tunnel_dscp_mode_t {
    PRESERVE_MODEL = 16w0,
    PIPE_MODEL = 16w1
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

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, inout standard_metadata_t standard_meta) {
    state start {
        packet.extract<ethernet_t>(hd.u0_ethernet);
        transition select(hd.u0_ethernet.ether_type) {
            16w0x800: parse_u0_ipv4;
            16w0x86dd: parse_u0_ipv6;
            default: accept;
        }
    }
    state parse_u0_ipv4 {
        packet.extract<ipv4_t>(hd.u0_ipv4);
        verify(hd.u0_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.u0_ipv4.ihl >= 4w5, error.InvalidIPv4Header);
        transition select(hd.u0_ipv4.ihl) {
            4w5: dispatch_on_u0_protocol;
            default: parse_u0_ipv4options;
        }
    }
    state parse_u0_ipv4options {
        packet.extract<ipv4options_t>(hd.u0_ipv4options, (bit<32>)((bit<16>)hd.u0_ipv4.ihl + 16w65531 << 5));
        transition dispatch_on_u0_protocol;
    }
    state dispatch_on_u0_protocol {
        transition select(hd.u0_ipv4.protocol) {
            8w17: parse_u0_udp;
            8w6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_ipv6 {
        packet.extract<ipv6_t>(hd.u0_ipv6);
        transition select(hd.u0_ipv6.next_header) {
            8w17: parse_u0_udp;
            8w6: parse_u0_tcp;
            default: accept;
        }
    }
    state parse_u0_udp {
        packet.extract<udp_t>(hd.u0_udp);
        transition select(hd.u0_udp.dst_port) {
            16w4789: parse_u0_vxlan;
            default: accept;
        }
    }
    state parse_u0_tcp {
        packet.extract<tcp_t>(hd.u0_tcp);
        transition accept;
    }
    state parse_u0_vxlan {
        packet.extract<vxlan_t>(hd.u0_vxlan);
        transition parse_customer_ethernet;
    }
    state parse_customer_ethernet {
        packet.extract<ethernet_t>(hd.customer_ethernet);
        transition select(hd.customer_ethernet.ether_type) {
            16w0x800: parse_customer_ipv4;
            16w0x86dd: parse_customer_ipv6;
            default: accept;
        }
    }
    state parse_customer_ipv4 {
        packet.extract<ipv4_t>(hd.customer_ipv4);
        verify(hd.customer_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.customer_ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hd.customer_ipv4.protocol) {
            8w17: parse_customer_udp;
            8w6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_ipv6 {
        packet.extract<ipv6_t>(hd.customer_ipv6);
        transition select(hd.customer_ipv6.next_header) {
            8w17: parse_customer_udp;
            8w6: parse_customer_tcp;
            default: accept;
        }
    }
    state parse_customer_tcp {
        packet.extract<tcp_t>(hd.customer_tcp);
        transition accept;
    }
    state parse_customer_udp {
        packet.extract<udp_t>(hd.customer_udp);
        transition accept;
    }
}

control dash_deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.u0_ethernet);
        packet.emit<ipv4_t>(hdr.u0_ipv4);
        packet.emit<ipv4options_t>(hdr.u0_ipv4options);
        packet.emit<ipv6_t>(hdr.u0_ipv6);
        packet.emit<udp_t>(hdr.u0_udp);
        packet.emit<tcp_t>(hdr.u0_tcp);
        packet.emit<vxlan_t>(hdr.u0_vxlan);
        packet.emit<nvgre_t>(hdr.u0_nvgre);
        packet.emit<ethernet_t>(hdr.customer_ethernet);
        packet.emit<ipv4_t>(hdr.customer_ipv4);
        packet.emit<ipv6_t>(hdr.customer_ipv6);
        packet.emit<tcp_t>(hdr.customer_tcp);
        packet.emit<udp_t>(hdr.customer_udp);
    }
}

action push_vxlan_tunnel_u0(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.customer_ethernet.dst_addr = (overlay_dmac == 48w0 ? hdr.customer_ethernet.dst_addr : overlay_dmac);
    hdr.u0_ethernet.setValid();
    hdr.u0_ethernet.dst_addr = underlay_dmac;
    hdr.u0_ethernet.src_addr = underlay_smac;
    hdr.u0_ethernet.ether_type = 16w0x800;
    hdr.u0_ipv4.setValid();
    hdr.u0_ipv4.total_len = hdr.customer_ipv4.total_len * (bit<16>)(bit<1>)hdr.customer_ipv4.isValid() + hdr.customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w50;
    hdr.u0_ipv4.version = 4w4;
    hdr.u0_ipv4.ihl = 4w5;
    hdr.u0_ipv4.diffserv = 8w0;
    hdr.u0_ipv4.identification = 16w1;
    hdr.u0_ipv4.flags = 3w0;
    hdr.u0_ipv4.frag_offset = 13w0;
    hdr.u0_ipv4.ttl = 8w64;
    hdr.u0_ipv4.protocol = 8w17;
    hdr.u0_ipv4.dst_addr = underlay_dip;
    hdr.u0_ipv4.src_addr = underlay_sip;
    hdr.u0_ipv4.hdr_checksum = 16w0;
    hdr.u0_udp.setValid();
    hdr.u0_udp.src_port = 16w0;
    hdr.u0_udp.dst_port = 16w4789;
    hdr.u0_udp.length = hdr.customer_ipv4.total_len * (bit<16>)(bit<1>)hdr.customer_ipv4.isValid() + hdr.customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w30;
    hdr.u0_udp.checksum = 16w0;
    hdr.u0_vxlan.setValid();
    hdr.u0_vxlan.reserved = 24w0;
    hdr.u0_vxlan.reserved_2 = 8w0;
    hdr.u0_vxlan.flags = 8w0x8;
    hdr.u0_vxlan.vni = tunnel_key;
}
action push_vxlan_tunnel_u1(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.u0_ethernet.dst_addr = (overlay_dmac == 48w0 ? hdr.u0_ethernet.dst_addr : overlay_dmac);
    hdr.u1_ethernet.setValid();
    hdr.u1_ethernet.dst_addr = underlay_dmac;
    hdr.u1_ethernet.src_addr = underlay_smac;
    hdr.u1_ethernet.ether_type = 16w0x800;
    hdr.u1_ipv4.setValid();
    hdr.u1_ipv4.total_len = hdr.u0_ipv4.total_len * (bit<16>)(bit<1>)hdr.u0_ipv4.isValid() + hdr.u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w50;
    hdr.u1_ipv4.version = 4w4;
    hdr.u1_ipv4.ihl = 4w5;
    hdr.u1_ipv4.diffserv = 8w0;
    hdr.u1_ipv4.identification = 16w1;
    hdr.u1_ipv4.flags = 3w0;
    hdr.u1_ipv4.frag_offset = 13w0;
    hdr.u1_ipv4.ttl = 8w64;
    hdr.u1_ipv4.protocol = 8w17;
    hdr.u1_ipv4.dst_addr = underlay_dip;
    hdr.u1_ipv4.src_addr = underlay_sip;
    hdr.u1_ipv4.hdr_checksum = 16w0;
    hdr.u1_udp.setValid();
    hdr.u1_udp.src_port = 16w0;
    hdr.u1_udp.dst_port = 16w4789;
    hdr.u1_udp.length = hdr.u0_ipv4.total_len * (bit<16>)(bit<1>)hdr.u0_ipv4.isValid() + hdr.u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w30;
    hdr.u1_udp.checksum = 16w0;
    hdr.u1_vxlan.setValid();
    hdr.u1_vxlan.reserved = 24w0;
    hdr.u1_vxlan.reserved_2 = 8w0;
    hdr.u1_vxlan.flags = 8w0x8;
    hdr.u1_vxlan.vni = tunnel_key;
}
action push_nvgre_tunnel_u0(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.customer_ethernet.dst_addr = overlay_dmac;
    hdr.u0_ethernet.setValid();
    hdr.u0_ethernet.dst_addr = underlay_dmac;
    hdr.u0_ethernet.src_addr = underlay_smac;
    hdr.u0_ethernet.ether_type = 16w0x800;
    hdr.u0_ipv4.setValid();
    hdr.u0_ipv4.total_len = hdr.customer_ipv4.total_len * (bit<16>)(bit<1>)hdr.customer_ipv4.isValid() + hdr.customer_ipv6.payload_length * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.customer_ipv6.isValid() + 16w42;
    hdr.u0_ipv4.total_len = 16w42 + hdr.u0_ipv4.total_len;
    hdr.u0_ipv4.version = 4w4;
    hdr.u0_ipv4.ihl = 4w5;
    hdr.u0_ipv4.diffserv = 8w0;
    hdr.u0_ipv4.identification = 16w1;
    hdr.u0_ipv4.flags = 3w0;
    hdr.u0_ipv4.frag_offset = 13w0;
    hdr.u0_ipv4.ttl = 8w64;
    hdr.u0_ipv4.protocol = 8w0x2f;
    hdr.u0_ipv4.dst_addr = underlay_dip;
    hdr.u0_ipv4.src_addr = underlay_sip;
    hdr.u0_ipv4.hdr_checksum = 16w0;
    hdr.u0_nvgre.setValid();
    hdr.u0_nvgre.flags = 4w4;
    hdr.u0_nvgre.reserved = 9w0;
    hdr.u0_nvgre.version = 3w0;
    hdr.u0_nvgre.protocol_type = 16w0x6558;
    hdr.u0_nvgre.vsid = tunnel_key;
    hdr.u0_nvgre.flow_id = 8w0;
}
action push_nvgre_tunnel_u1(inout headers_t hdr, in EthernetAddress overlay_dmac, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in bit<24> tunnel_key) {
    hdr.u0_ethernet.dst_addr = overlay_dmac;
    hdr.u1_ethernet.setValid();
    hdr.u1_ethernet.dst_addr = underlay_dmac;
    hdr.u1_ethernet.src_addr = underlay_smac;
    hdr.u1_ethernet.ether_type = 16w0x800;
    hdr.u1_ipv4.setValid();
    hdr.u1_ipv4.total_len = hdr.u0_ipv4.total_len * (bit<16>)(bit<1>)hdr.u0_ipv4.isValid() + hdr.u0_ipv6.payload_length * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.u0_ipv6.isValid() + 16w42;
    hdr.u1_ipv4.total_len = 16w42 + hdr.u1_ipv4.total_len;
    hdr.u1_ipv4.version = 4w4;
    hdr.u1_ipv4.ihl = 4w5;
    hdr.u1_ipv4.diffserv = 8w0;
    hdr.u1_ipv4.identification = 16w1;
    hdr.u1_ipv4.flags = 3w0;
    hdr.u1_ipv4.frag_offset = 13w0;
    hdr.u1_ipv4.ttl = 8w64;
    hdr.u1_ipv4.protocol = 8w0x2f;
    hdr.u1_ipv4.dst_addr = underlay_dip;
    hdr.u1_ipv4.src_addr = underlay_sip;
    hdr.u1_ipv4.hdr_checksum = 16w0;
    hdr.u1_nvgre.setValid();
    hdr.u1_nvgre.flags = 4w4;
    hdr.u1_nvgre.reserved = 9w0;
    hdr.u1_nvgre.version = 3w0;
    hdr.u1_nvgre.protocol_type = 16w0x6558;
    hdr.u1_nvgre.vsid = tunnel_key;
    hdr.u1_nvgre.flow_id = 8w0;
}
action tunnel_decap(inout headers_t hdr, inout metadata_t meta) {
    hdr.u0_ethernet.setInvalid();
    hdr.u0_ipv4.setInvalid();
    hdr.u0_ipv6.setInvalid();
    hdr.u0_nvgre.setInvalid();
    hdr.u0_vxlan.setInvalid();
    hdr.u0_udp.setInvalid();
    meta.tunnel_pointer = 16w0;
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
    direct_counter(CounterType.packets_and_bytes) stage1_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", order=1, isobject="true"] table stage1 {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage1_dash_acl_group_id");
            meta.dst_ip_addr             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta.ip_protocol             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta.src_l4_port             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta.dst_l4_port             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            permit();
            permit_and_continue();
            deny();
            deny_and_continue();
        }
        default_action = deny();
        counters = stage1_counter;
    }
    direct_counter(CounterType.packets_and_bytes) stage2_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", order=1, isobject="true"] table stage2 {
        key = {
            meta.stage2_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage2_dash_acl_group_id");
            meta.dst_ip_addr             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta.ip_protocol             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta.src_l4_port             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta.dst_l4_port             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            permit();
            permit_and_continue();
            deny();
            deny_and_continue();
        }
        default_action = deny();
        counters = stage2_counter;
    }
    direct_counter(CounterType.packets_and_bytes) stage3_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", order=1, isobject="true"] table stage3 {
        key = {
            meta.stage3_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage3_dash_acl_group_id");
            meta.dst_ip_addr             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta.ip_protocol             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta.src_l4_port             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta.dst_l4_port             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            permit();
            permit_and_continue();
            deny();
            deny_and_continue();
        }
        default_action = deny();
        counters = stage3_counter;
    }
    apply {
        if (meta.stage1_dash_acl_group_id != 16w0) {
            switch (stage1.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
                default: {
                }
            }
        }
        if (meta.stage2_dash_acl_group_id != 16w0) {
            switch (stage2.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
                default: {
                }
            }
        }
        if (meta.stage3_dash_acl_group_id != 16w0) {
            switch (stage3.apply().action_run) {
                permit: {
                    return;
                }
                deny: {
                    return;
                }
                default: {
                }
            }
        }
    }
}

action push_action_static_encap(in headers_t hdr, inout metadata_t meta, in dash_encapsulation_t encap=dash_encapsulation_t.VXLAN, in bit<24> vni=0, in IPv4Address underlay_sip=0, in IPv4Address underlay_dip=0, in EthernetAddress underlay_smac=0, in EthernetAddress underlay_dmac=0, in EthernetAddress overlay_dmac=0) {
    meta.routing_actions = meta.routing_actions | 32w1;
    meta.encap_data.dash_encapsulation = encap;
    meta.encap_data.vni = (vni == 24w0 ? meta.encap_data.vni : vni);
    meta.encap_data.underlay_smac = (underlay_smac == 48w0 ? meta.encap_data.underlay_smac : underlay_smac);
    meta.encap_data.underlay_dmac = (underlay_dmac == 48w0 ? meta.encap_data.underlay_dmac : underlay_dmac);
    meta.encap_data.underlay_sip = (underlay_sip == 32w0 ? meta.encap_data.underlay_sip : underlay_sip);
    meta.encap_data.underlay_dip = (underlay_dip == 32w0 ? meta.encap_data.underlay_dip : underlay_dip);
    meta.overlay_data.dmac = (overlay_dmac == 48w0 ? meta.overlay_data.dmac : overlay_dmac);
}
control do_action_static_encap(inout headers_t hdr, inout metadata_t meta) {
    apply {
        if (meta.routing_actions & 32w1 == 32w0) {
            return;
        }
        if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
            if (meta.tunnel_pointer == 16w0) {
                push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            } else if (meta.tunnel_pointer == 16w1) {
                push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            }
        } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
            if (meta.tunnel_pointer == 16w0) {
                push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            } else if (meta.tunnel_pointer == 16w1) {
                push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
            }
        }
        meta.tunnel_pointer = meta.tunnel_pointer + 16w1;
    }
}

action push_action_nat46(in headers_t hdr, inout metadata_t meta, in IPv6Address sip, in IPv6Address sip_mask, in IPv6Address dip, in IPv6Address dip_mask) {
    meta.routing_actions = meta.routing_actions | 32w2;
    meta.overlay_data.is_ipv6 = true;
    meta.overlay_data.sip = sip;
    meta.overlay_data.sip_mask = sip_mask;
    meta.overlay_data.dip = dip;
    meta.overlay_data.dip_mask = dip_mask;
}
control do_action_nat46(inout headers_t hdr, in metadata_t meta) {
    apply {
        if (meta.routing_actions & 32w2 == 32w0) {
            return;
        }
        assert(meta.overlay_data.is_ipv6);
        hdr.u0_ipv6.setValid();
        hdr.u0_ipv6.version = 4w6;
        hdr.u0_ipv6.traffic_class = 8w0;
        hdr.u0_ipv6.flow_label = 20w0;
        hdr.u0_ipv6.payload_length = hdr.u0_ipv4.total_len + 16w65516;
        hdr.u0_ipv6.next_header = hdr.u0_ipv4.protocol;
        hdr.u0_ipv6.hop_limit = hdr.u0_ipv4.ttl;
        hdr.u0_ipv6.dst_addr = (IPv6Address)hdr.u0_ipv4.dst_addr & ~meta.overlay_data.dip_mask | meta.overlay_data.dip & meta.overlay_data.dip_mask;
        hdr.u0_ipv6.src_addr = (IPv6Address)hdr.u0_ipv4.src_addr & ~meta.overlay_data.sip_mask | meta.overlay_data.sip & meta.overlay_data.sip_mask;
        hdr.u0_ipv4.setInvalid();
        hdr.u0_ethernet.ether_type = 16w0x86dd;
    }
}

action push_action_nat64(in headers_t hdr, inout metadata_t meta, in IPv4Address src, in IPv4Address dst) {
    meta.routing_actions = meta.routing_actions | 32w4;
    meta.overlay_data.is_ipv6 = false;
    meta.overlay_data.sip = (IPv4ORv6Address)src;
    meta.overlay_data.dip = (IPv4ORv6Address)dst;
}
control do_action_nat64(inout headers_t hdr, in metadata_t meta) {
    apply {
        if (meta.routing_actions & 32w4 == 32w0) {
            return;
        }
        assert(!meta.overlay_data.is_ipv6);
        hdr.u0_ipv4.setValid();
        hdr.u0_ipv4.version = 4w4;
        hdr.u0_ipv4.ihl = 4w5;
        hdr.u0_ipv4.diffserv = 8w0;
        hdr.u0_ipv4.total_len = hdr.u0_ipv6.payload_length + 16w20;
        hdr.u0_ipv4.identification = 16w1;
        hdr.u0_ipv4.flags = 3w0;
        hdr.u0_ipv4.frag_offset = 13w0;
        hdr.u0_ipv4.protocol = hdr.u0_ipv6.next_header;
        hdr.u0_ipv4.ttl = hdr.u0_ipv6.hop_limit;
        hdr.u0_ipv4.hdr_checksum = 16w0;
        hdr.u0_ipv4.dst_addr = (IPv4Address)meta.overlay_data.dip;
        hdr.u0_ipv4.src_addr = (IPv4Address)meta.overlay_data.sip;
        hdr.u0_ipv6.setInvalid();
        hdr.u0_ethernet.ether_type = 16w0x800;
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
    push_action_static_encap(hdr = hdr, meta = meta, encap = dash_encapsulation, vni = tunnel_key, underlay_sip = (underlay_sip == 128w0 ? hdr.u0_ipv4.src_addr : (IPv4Address)underlay_sip), underlay_dip = (underlay_dip == 128w0 ? hdr.u0_ipv4.dst_addr : (IPv4Address)underlay_dip), underlay_smac = 48w0, underlay_dmac = 48w0, overlay_dmac = hdr.u0_ethernet.dst_addr);
    set_route_meter_attrs(meta, meter_policy_en, meter_class);
}
action set_tunnel_mapping(inout headers_t hdr, inout metadata_t meta, @SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, EthernetAddress overlay_dmac, bit<1> use_dst_vnet_vni, bit<16> meter_class, bit<1> meter_class_override) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    if (use_dst_vnet_vni == 1w1) {
        meta.vnet_id = meta.dst_vnet_id;
    }
    push_action_static_encap(hdr = hdr, meta = meta, encap = dash_encapsulation_t.VXLAN, vni = 24w0, underlay_sip = 32w0, underlay_dip = underlay_dip, underlay_smac = 48w0, underlay_dmac = 48w0, overlay_dmac = overlay_dmac);
    set_mapping_meter_attr(meta, meter_class, meter_class_override);
}
action set_private_link_mapping(inout headers_t hdr, inout metadata_t meta, @SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, IPv6Address overlay_sip, IPv6Address overlay_dip, @SaiVal[type="sai_dash_encapsulation_t"] dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<16> meter_class, bit<1> meter_class_override) {
    meta.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
    push_action_static_encap(hdr = hdr, meta = meta, encap = dash_encapsulation, vni = tunnel_key, underlay_sip = meta.eni_data.pl_underlay_sip, underlay_dip = underlay_dip, underlay_smac = 48w0, underlay_dmac = 48w0, overlay_dmac = hdr.u0_ethernet.dst_addr);
    push_action_nat46(hdr = hdr, meta = meta, dip = overlay_dip, dip_mask = 128w0xffffffffffffffffffffffff, sip = overlay_sip & ~meta.eni_data.pl_sip_mask | meta.eni_data.pl_sip | (IPv6Address)hdr.u0_ipv4.src_addr, sip_mask = 128w0xffffffffffffffffffffffff);
    set_mapping_meter_attr(meta, meter_class, meter_class_override);
}
control outbound_routing_stage(inout headers_t hdr, inout metadata_t meta) {
    direct_counter(CounterType.packets_and_bytes) routing_counter;
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] table routing {
        key = {
            meta.eni_id          : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.is_overlay_ip_v6: exact @SaiVal[name="destination_is_v6"] @name("meta.is_overlay_ip_v6");
            meta.dst_ip_addr     : lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            route_vnet(hdr, meta);
            route_vnet_direct(hdr, meta);
            route_direct(hdr, meta);
            route_service_tunnel(hdr, meta);
            drop(meta);
        }
        const default_action = drop(meta);
        counters = routing_counter;
    }
    apply {
        if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_ROUTING) {
            return;
        }
        routing.apply();
    }
}

control outbound_mapping_stage(inout headers_t hdr, inout metadata_t meta) {
    direct_counter(CounterType.packets_and_bytes) ca_to_pa_counter;
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] table ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @SaiVal[type="sai_object_id_t"] @name("meta.dst_vnet_id");
            meta.is_lkup_dst_ip_v6: exact @SaiVal[name="dip_is_v6"] @name("meta.is_lkup_dst_ip_v6");
            meta.lkup_dst_ip_addr : exact @SaiVal[name="dip"] @name("meta.lkup_dst_ip_addr");
        }
        actions = {
            set_tunnel_mapping(hdr, meta);
            set_private_link_mapping(hdr, meta);
            @defaultonly drop(meta);
        }
        const default_action = drop(meta);
        counters = ca_to_pa_counter;
    }
    action set_vnet_attrs(bit<24> vni) {
        meta.encap_data.vni = vni;
    }
    @SaiTable[name="vnet", api="dash_vnet", isobject="true"] table vnet {
        key = {
            meta.vnet_id: exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
        }
        actions = {
            set_vnet_attrs();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_MAPPING) {
            return;
        }
        switch (ca_to_pa.apply().action_run) {
            set_tunnel_mapping: {
                vnet.apply();
            }
            default: {
            }
        }
    }
}

control outbound(inout headers_t hdr, inout metadata_t meta) {
    @name("acl") acl() acl_inst;
    @name("outbound_routing_stage") outbound_routing_stage() outbound_routing_stage_inst;
    @name("outbound_mapping_stage") outbound_mapping_stage() outbound_mapping_stage_inst;
    apply {
        if (meta.conntrack_data.allow_out) {
            ;
        } else {
            acl_inst.apply(hdr, meta);
        }
        meta.lkup_dst_ip_addr = meta.dst_ip_addr;
        meta.is_lkup_dst_ip_v6 = meta.is_overlay_ip_v6;
        outbound_routing_stage_inst.apply(hdr, meta);
        outbound_mapping_stage_inst.apply(hdr, meta);
    }
}

action service_tunnel_encode(inout headers_t hdr, in IPv6Address st_dst, in IPv6Address st_dst_mask, in IPv6Address st_src, in IPv6Address st_src_mask) {
    hdr.u0_ipv6.setValid();
    hdr.u0_ipv6.version = 4w6;
    hdr.u0_ipv6.traffic_class = 8w0;
    hdr.u0_ipv6.flow_label = 20w0;
    hdr.u0_ipv6.payload_length = hdr.u0_ipv4.total_len + 16w65516;
    hdr.u0_ipv6.next_header = hdr.u0_ipv4.protocol;
    hdr.u0_ipv6.hop_limit = hdr.u0_ipv4.ttl;
    hdr.u0_ipv6.dst_addr = (IPv6Address)hdr.u0_ipv4.dst_addr & ~st_dst_mask | st_dst & st_dst_mask;
    hdr.u0_ipv6.src_addr = (IPv6Address)hdr.u0_ipv4.src_addr & ~st_src_mask | st_src & st_src_mask;
    hdr.u0_ipv4.setInvalid();
    hdr.u0_ethernet.ether_type = 16w0x86dd;
}
action service_tunnel_decode(inout headers_t hdr, in IPv4Address src, in IPv4Address dst) {
    hdr.u0_ipv4.setValid();
    hdr.u0_ipv4.version = 4w4;
    hdr.u0_ipv4.ihl = 4w5;
    hdr.u0_ipv4.diffserv = 8w0;
    hdr.u0_ipv4.total_len = hdr.u0_ipv6.payload_length + 16w20;
    hdr.u0_ipv4.identification = 16w1;
    hdr.u0_ipv4.flags = 3w0;
    hdr.u0_ipv4.frag_offset = 13w0;
    hdr.u0_ipv4.protocol = hdr.u0_ipv6.next_header;
    hdr.u0_ipv4.ttl = hdr.u0_ipv6.hop_limit;
    hdr.u0_ipv4.hdr_checksum = 16w0;
    hdr.u0_ipv4.dst_addr = dst;
    hdr.u0_ipv4.src_addr = src;
    hdr.u0_ipv6.setInvalid();
    hdr.u0_ethernet.ether_type = 16w0x800;
}
control inbound(inout headers_t hdr, inout metadata_t meta) {
    @name("acl") acl() acl_inst_0;
    apply {
        if (meta.conntrack_data.allow_in) {
            ;
        } else {
            acl_inst_0.apply(hdr, meta);
        }
        if (meta.tunnel_pointer == 16w0) {
            push_vxlan_tunnel_u0(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
        } else if (meta.tunnel_pointer == 16w1) {
            push_vxlan_tunnel_u1(hdr, meta.overlay_data.dmac, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.vni);
        }
        meta.tunnel_pointer = meta.tunnel_pointer + 16w1;
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
            hdr.u0_vxlan.vni: exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
        }
        actions = {
            set_outbound_direction();
            @defaultonly set_inbound_direction();
        }
        const default_action = set_inbound_direction();
    }
    apply {
        direction_lookup.apply();
    }
}

control eni_lookup_stage(inout headers_t hdr, inout metadata_t meta) {
    @SaiCounter[name="lb_fast_path_eni_miss", attr_type="stats"] counter(32w1, CounterType.packets_and_bytes) port_lb_fast_path_eni_miss_counter;
    action set_eni(@SaiVal[type="sai_object_id_t"] bit<16> eni_id) {
        meta.eni_id = eni_id;
    }
    action deny() {
        meta.dropped = true;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", order=0] table eni_ether_address_map {
        key = {
            meta.eni_addr: exact @SaiVal[name="address", type="sai_mac_t"] @name("meta.eni_addr");
        }
        actions = {
            set_eni();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    apply {
        meta.eni_addr = (meta.direction == dash_direction_t.OUTBOUND ? hdr.customer_ethernet.src_addr : hdr.customer_ethernet.dst_addr);
        if (eni_ether_address_map.apply().hit) {
            ;
        } else if (meta.is_fast_path_icmp_flow_redirection_packet) {
            port_lb_fast_path_eni_miss_counter.count(32w0);
        }
    }
}

control routing_action_apply(inout headers_t hdr, inout metadata_t meta) {
    @name("do_action_nat46") do_action_nat46() do_action_nat46_inst;
    @name("do_action_nat64") do_action_nat64() do_action_nat64_inst;
    @name("do_action_static_encap") do_action_static_encap() do_action_static_encap_inst;
    apply {
        do_action_nat46_inst.apply(hdr, meta);
        do_action_nat64_inst.apply(hdr, meta);
        do_action_static_encap_inst.apply(hdr, meta);
    }
}

control metering_update_stage(inout headers_t hdr, inout metadata_t meta) {
    action check_ip_addr_family(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", order=1, isobject="true"] table meter_policy {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id");
        }
        actions = {
            check_ip_addr_family();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    action set_policy_meter_class(bit<16> meter_class) {
        meta.policy_meter_class = meter_class;
    }
    @SaiTable[name="meter_rule", api="dash_meter", order=2, isobject="true"] table meter_rule {
        key = {
            meta.meter_policy_id: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"] @name("meta.meter_policy_id");
            hdr.u0_ipv4.dst_addr: ternary @SaiVal[name="dip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            set_policy_meter_class();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
    }
    @SaiCounter[name="outbound", action_names="meter_bucket_action", attr_type="counter_attr"] counter(32w262144, CounterType.bytes) meter_bucket_outbound;
    @SaiCounter[name="inbound", action_names="meter_bucket_action", attr_type="counter_attr"] counter(32w262144, CounterType.bytes) meter_bucket_inbound;
    action meter_bucket_action(@SaiVal[type="sai_uint32_t", skipattr="true"] bit<32> meter_bucket_index) {
        meta.meter_bucket_index = meter_bucket_index;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", order=0, isobject="true"] table meter_bucket {
        key = {
            meta.eni_id     : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.meter_class: exact @name("meta.meter_class");
        }
        actions = {
            meter_bucket_action();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
    }
    direct_counter(CounterType.packets_and_bytes) eni_counter;
    @SaiTable[ignored="true"] table eni_meter {
        key = {
            meta.eni_id   : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.direction: exact @name("meta.direction");
            meta.dropped  : exact @name("meta.dropped");
        }
        actions = {
            NoAction();
        }
        counters = eni_counter;
        default_action = NoAction();
    }
    apply {
        if (meta.meter_policy_en == 1w1) {
            meter_policy.apply();
            meter_rule.apply();
        }
        if (meta.meter_policy_en == 1w1) {
            meta.meter_class = meta.policy_meter_class;
        } else {
            meta.meter_class = meta.route_meter_class;
        }
        if (meta.meter_class == 16w0 || meta.mapping_meter_class_override == 1w1) {
            meta.meter_class = meta.mapping_meter_class;
        }
        meter_bucket.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            meter_bucket_outbound.count(meta.meter_bucket_index);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            meter_bucket_inbound.count(meta.meter_bucket_index);
        }
        eni_meter.apply();
    }
}

control underlay(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    action set_nhop(bit<9> next_hop_id) {
        standard_metadata.egress_spec = next_hop_id;
    }
    action pkt_act(bit<9> packet_action, bit<9> next_hop_id) {
        if (packet_action == 9w0) {
            meta.dropped = true;
        } else if (packet_action == 9w1) {
            set_nhop(next_hop_id);
        }
    }
    action def_act() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @SaiTable[name="route", api="route", api_type="underlay"] table underlay_routing {
        key = {
            meta.dst_ip_addr: lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            pkt_act();
            @defaultonly def_act();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        underlay_routing.apply();
    }
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    action drop_action() {
        mark_to_drop(standard_metadata);
    }
    action deny() {
        meta.dropped = true;
    }
    action accept() {
    }
    @SaiCounter[name="lb_fast_path_icmp_in", attr_type="stats"] counter(32w1, CounterType.packets_and_bytes) port_lb_fast_path_icmp_in_counter;
    @SaiTable[name="vip", api="dash_vip"] table vip {
        key = {
            hdr.u0_ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            accept();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    action set_appliance(EthernetAddress neighbor_mac, EthernetAddress mac) {
        meta.encap_data.underlay_dmac = neighbor_mac;
        meta.encap_data.underlay_smac = mac;
    }
    @SaiTable[ignored="true"] table appliance {
        key = {
            meta.appliance_id: ternary @name("meta.appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @SaiCounter[name="lb_fast_path_icmp_in", attr_type="stats", action_names="set_eni_attrs"] counter(32w64, CounterType.packets_and_bytes) eni_lb_fast_path_icmp_in_counter;
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
        if (meta.is_overlay_ip_v6 == 1w1) {
            if (meta.direction == dash_direction_t.OUTBOUND) {
                meta.stage1_dash_acl_group_id = outbound_v6_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = outbound_v6_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = outbound_v6_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = outbound_v6_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = outbound_v6_stage5_dash_acl_group_id;
            } else {
                meta.stage1_dash_acl_group_id = inbound_v6_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = inbound_v6_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = inbound_v6_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = inbound_v6_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = inbound_v6_stage5_dash_acl_group_id;
            }
            meta.meter_policy_id = v6_meter_policy_id;
        } else {
            if (meta.direction == dash_direction_t.OUTBOUND) {
                meta.stage1_dash_acl_group_id = outbound_v4_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = outbound_v4_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = outbound_v4_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = outbound_v4_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = outbound_v4_stage5_dash_acl_group_id;
            } else {
                meta.stage1_dash_acl_group_id = inbound_v4_stage1_dash_acl_group_id;
                meta.stage2_dash_acl_group_id = inbound_v4_stage2_dash_acl_group_id;
                meta.stage3_dash_acl_group_id = inbound_v4_stage3_dash_acl_group_id;
                meta.stage4_dash_acl_group_id = inbound_v4_stage4_dash_acl_group_id;
                meta.stage5_dash_acl_group_id = inbound_v4_stage5_dash_acl_group_id;
            }
            meta.meter_policy_id = v4_meter_policy_id;
        }
        meta.fast_path_icmp_flow_redirection_disabled = disable_fast_path_icmp_flow_redirection;
    }
    @SaiTable[name="eni", api="dash_eni", order=1, isobject="true"] table eni {
        key = {
            meta.eni_id: exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    action permit() {
    }
    action tunnel_decap_pa_validate(@SaiVal[type="sai_object_id_t"] bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] table pa_validation {
        key = {
            meta.vnet_id        : exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
            hdr.u0_ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            permit();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] table inbound_routing {
        key = {
            meta.eni_id         : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            hdr.u0_vxlan.vni    : exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
            hdr.u0_ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            tunnel_decap(hdr, meta);
            tunnel_decap_pa_validate();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    action set_acl_group_attrs(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @SaiTable[name="dash_acl_group", api="dash_acl", order=0, isobject="true"] table acl_group {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name("direction_lookup_stage") direction_lookup_stage() direction_lookup_stage_inst;
    @name("eni_lookup_stage") eni_lookup_stage() eni_lookup_stage_inst;
    @name("outbound") outbound() outbound_inst;
    @name("inbound") inbound() inbound_inst;
    @name("routing_action_apply") routing_action_apply() routing_action_apply_inst;
    @name("underlay") underlay() underlay_inst;
    @name("metering_update_stage") metering_update_stage() metering_update_stage_inst;
    apply {
        if (meta.is_fast_path_icmp_flow_redirection_packet) {
            port_lb_fast_path_icmp_in_counter.count(32w0);
        }
        if (vip.apply().hit) {
            meta.encap_data.underlay_sip = hdr.u0_ipv4.dst_addr;
        } else {
            ;
        }
        direction_lookup_stage_inst.apply(hdr, meta);
        appliance.apply();
        eni_lookup_stage_inst.apply(hdr, meta);
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
                default: {
                }
            }
        }
        meta.is_overlay_ip_v6 = 1w0;
        meta.ip_protocol = 8w0;
        meta.dst_ip_addr = 128w0;
        meta.src_ip_addr = 128w0;
        if (hdr.customer_ipv6.isValid()) {
            meta.ip_protocol = hdr.customer_ipv6.next_header;
            meta.src_ip_addr = hdr.customer_ipv6.src_addr;
            meta.dst_ip_addr = hdr.customer_ipv6.dst_addr;
            meta.is_overlay_ip_v6 = 1w1;
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
        if (meta.eni_data.admin_state == 1w0) {
            deny();
        }
        if (meta.is_fast_path_icmp_flow_redirection_packet) {
            eni_lb_fast_path_icmp_in_counter.count((bit<32>)meta.eni_id);
        }
        acl_group.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            meta.target_stage = dash_pipeline_stage_t.OUTBOUND_ROUTING;
            outbound_inst.apply(hdr, meta);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            inbound_inst.apply(hdr, meta);
        }
        routing_action_apply_inst.apply(hdr, meta);
        meta.dst_ip_addr = (bit<128>)hdr.u0_ipv4.dst_addr;
        underlay_inst.apply(hdr, meta, standard_metadata);
        if (meta.eni_data.dscp_mode == dash_tunnel_dscp_mode_t.PIPE_MODEL) {
            hdr.u0_ipv4.diffserv = (bit<8>)meta.eni_data.dscp;
        }
        metering_update_stage_inst.apply(hdr, meta);
        if (meta.dropped) {
            drop_action();
        }
    }
}

control dash_verify_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control dash_compute_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<13>, bit<3>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.u0_ipv4.isValid(), { hdr.u0_ipv4.version, hdr.u0_ipv4.ihl, hdr.u0_ipv4.diffserv, hdr.u0_ipv4.total_len, hdr.u0_ipv4.identification, hdr.u0_ipv4.frag_offset, hdr.u0_ipv4.flags, hdr.u0_ipv4.ttl, hdr.u0_ipv4.protocol, hdr.u0_ipv4.src_addr, hdr.u0_ipv4.dst_addr }, hdr.u0_ipv4.hdr_checksum, HashAlgorithm.csum16);
    }
}

control dash_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(dash_parser(), dash_verify_checksum(), dash_ingress(), dash_egress(), dash_compute_checksum(), dash_deparser()) main;
