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
    ethernet_t    ethernet;
    ipv4_t        ipv4;
    ipv4options_t ipv4options;
    ipv6_t        ipv6;
    udp_t         udp;
    tcp_t         tcp;
    vxlan_t       vxlan;
    nvgre_t       nvgre;
    ethernet_t    inner_ethernet;
    ipv4_t        inner_ipv4;
    ipv6_t        inner_ipv6;
    udp_t         inner_udp;
    tcp_t         inner_tcp;
}

enum bit<16> dash_encapsulation_t {
    INVALID = 16w0,
    VXLAN = 16w1,
    NVGRE = 16w2
}

struct encap_data_t {
    bit<24>              vni;
    bit<24>              dest_vnet_vni;
    IPv4Address          underlay_sip;
    IPv4Address          underlay_dip;
    EthernetAddress      underlay_smac;
    EthernetAddress      underlay_dmac;
    EthernetAddress      overlay_dmac;
    dash_encapsulation_t dash_encapsulation;
    bit<24>              service_tunnel_key;
    IPv4Address          original_overlay_sip;
    IPv4Address          original_overlay_dip;
}

enum bit<16> dash_direction_t {
    INVALID = 16w0,
    OUTBOUND = 16w1,
    INBOUND = 16w2
}

struct conntrack_data_t {
    bool allow_in;
    bool allow_out;
}

struct eni_data_t {
    bit<32>     cps;
    bit<32>     pps;
    bit<32>     flows;
    bit<1>      admin_state;
    IPv6Address pl_sip;
    IPv6Address pl_sip_mask;
    IPv4Address pl_underlay_sip;
}

struct metadata_t {
    bool             dropped;
    dash_direction_t direction;
    encap_data_t     encap_data;
    EthernetAddress  eni_addr;
    bit<16>          vnet_id;
    bit<16>          dst_vnet_id;
    bit<16>          eni_id;
    eni_data_t       eni_data;
    bit<16>          inbound_vm_id;
    bit<8>           appliance_id;
    bit<1>           is_overlay_ip_v6;
    bit<1>           is_lkup_dst_ip_v6;
    bit<8>           ip_protocol;
    IPv4ORv6Address  dst_ip_addr;
    IPv4ORv6Address  src_ip_addr;
    IPv4ORv6Address  lkup_dst_ip_addr;
    conntrack_data_t conntrack_data;
    bit<16>          src_l4_port;
    bit<16>          dst_l4_port;
    bit<16>          stage1_dash_acl_group_id;
    bit<16>          stage2_dash_acl_group_id;
    bit<16>          stage3_dash_acl_group_id;
    bit<16>          stage4_dash_acl_group_id;
    bit<16>          stage5_dash_acl_group_id;
    bit<1>           meter_policy_en;
    bit<1>           mapping_meter_class_override;
    bit<16>          meter_policy_id;
    bit<16>          policy_meter_class;
    bit<16>          route_meter_class;
    bit<16>          mapping_meter_class;
    bit<16>          meter_class;
    bit<32>          meter_bucket_index;
}

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, inout standard_metadata_t standard_meta) {
    state start {
        packet.extract<ethernet_t>(hd.ethernet);
        transition select(hd.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hd.ipv4);
        verify(hd.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.ipv4.ihl >= 4w5, error.InvalidIPv4Header);
        transition select(hd.ipv4.ihl) {
            4w5: dispatch_on_protocol;
            default: parse_ipv4options;
        }
    }
    state parse_ipv4options {
        packet.extract<ipv4options_t>(hd.ipv4options, (bit<32>)((bit<16>)hd.ipv4.ihl + 16w65531 << 5));
        transition dispatch_on_protocol;
    }
    state dispatch_on_protocol {
        transition select(hd.ipv4.protocol) {
            8w17: parse_udp;
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract<ipv6_t>(hd.ipv6);
        transition select(hd.ipv6.next_header) {
            8w17: parse_udp;
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract<udp_t>(hd.udp);
        transition select(hd.udp.dst_port) {
            16w4789: parse_vxlan;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<tcp_t>(hd.tcp);
        transition accept;
    }
    state parse_vxlan {
        packet.extract<vxlan_t>(hd.vxlan);
        transition parse_inner_ethernet;
    }
    state parse_inner_ethernet {
        packet.extract<ethernet_t>(hd.inner_ethernet);
        transition select(hd.inner_ethernet.ether_type) {
            16w0x800: parse_inner_ipv4;
            16w0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract<ipv4_t>(hd.inner_ipv4);
        verify(hd.inner_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.inner_ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hd.inner_ipv4.protocol) {
            8w17: parse_inner_udp;
            8w6: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_ipv6 {
        packet.extract<ipv6_t>(hd.inner_ipv6);
        transition select(hd.inner_ipv6.next_header) {
            8w17: parse_inner_udp;
            8w6: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_tcp {
        packet.extract<tcp_t>(hd.inner_tcp);
        transition accept;
    }
    state parse_inner_udp {
        packet.extract<udp_t>(hd.inner_udp);
        transition accept;
    }
}

control dash_deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<ipv4options_t>(hdr.ipv4options);
        packet.emit<ipv6_t>(hdr.ipv6);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<vxlan_t>(hdr.vxlan);
        packet.emit<nvgre_t>(hdr.nvgre);
        packet.emit<ethernet_t>(hdr.inner_ethernet);
        packet.emit<ipv4_t>(hdr.inner_ipv4);
        packet.emit<ipv6_t>(hdr.inner_ipv6);
        packet.emit<tcp_t>(hdr.inner_tcp);
        packet.emit<udp_t>(hdr.inner_udp);
    }
}

action vxlan_encap(inout headers_t hdr, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in EthernetAddress overlay_dmac, in bit<24> vni) {
    hdr.inner_ethernet = hdr.ethernet;
    hdr.inner_ethernet.dst_addr = overlay_dmac;
    hdr.ethernet.setInvalid();
    hdr.inner_ipv4 = hdr.ipv4;
    hdr.ipv4.setInvalid();
    hdr.inner_ipv6 = hdr.ipv6;
    hdr.ipv6.setInvalid();
    hdr.inner_tcp = hdr.tcp;
    hdr.tcp.setInvalid();
    hdr.inner_udp = hdr.udp;
    hdr.udp.setInvalid();
    hdr.ethernet.setValid();
    hdr.ethernet.dst_addr = underlay_dmac;
    hdr.ethernet.src_addr = underlay_smac;
    hdr.ethernet.ether_type = 16w0x800;
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4w4;
    hdr.ipv4.ihl = 4w5;
    hdr.ipv4.diffserv = 8w0;
    hdr.ipv4.total_len = hdr.inner_ipv4.total_len * (bit<16>)(bit<1>)hdr.inner_ipv4.isValid() + hdr.inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w50;
    hdr.ipv4.identification = 16w1;
    hdr.ipv4.flags = 3w0;
    hdr.ipv4.frag_offset = 13w0;
    hdr.ipv4.ttl = 8w64;
    hdr.ipv4.protocol = 8w17;
    hdr.ipv4.dst_addr = underlay_dip;
    hdr.ipv4.src_addr = underlay_sip;
    hdr.ipv4.hdr_checksum = 16w0;
    hdr.udp.setValid();
    hdr.udp.src_port = 16w0;
    hdr.udp.dst_port = 16w4789;
    hdr.udp.length = hdr.inner_ipv4.total_len * (bit<16>)(bit<1>)hdr.inner_ipv4.isValid() + hdr.inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w30;
    hdr.udp.checksum = 16w0;
    hdr.vxlan.setValid();
    hdr.vxlan.reserved = 24w0;
    hdr.vxlan.reserved_2 = 8w0;
    hdr.vxlan.flags = 8w0;
    hdr.vxlan.vni = vni;
}
action vxlan_decap(inout headers_t hdr) {
    hdr.ethernet = hdr.inner_ethernet;
    hdr.inner_ethernet.setInvalid();
    hdr.ipv4 = hdr.inner_ipv4;
    hdr.inner_ipv4.setInvalid();
    hdr.ipv6 = hdr.inner_ipv6;
    hdr.inner_ipv6.setInvalid();
    hdr.vxlan.setInvalid();
    hdr.udp.setInvalid();
    hdr.tcp = hdr.inner_tcp;
    hdr.inner_tcp.setInvalid();
    hdr.udp = hdr.inner_udp;
    hdr.inner_udp.setInvalid();
}
action nvgre_encap(inout headers_t hdr, in EthernetAddress underlay_dmac, in EthernetAddress underlay_smac, in IPv4Address underlay_dip, in IPv4Address underlay_sip, in EthernetAddress overlay_dmac, in bit<24> vsid) {
    hdr.inner_ethernet = hdr.ethernet;
    hdr.inner_ethernet.dst_addr = overlay_dmac;
    hdr.ethernet.setInvalid();
    hdr.inner_ipv4 = hdr.ipv4;
    hdr.ipv4.setInvalid();
    hdr.inner_ipv6 = hdr.ipv6;
    hdr.ipv6.setInvalid();
    hdr.inner_tcp = hdr.tcp;
    hdr.tcp.setInvalid();
    hdr.inner_udp = hdr.udp;
    hdr.udp.setInvalid();
    hdr.ethernet.setValid();
    hdr.ethernet.dst_addr = underlay_dmac;
    hdr.ethernet.src_addr = underlay_smac;
    hdr.ethernet.ether_type = 16w0x800;
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4w4;
    hdr.ipv4.ihl = 4w5;
    hdr.ipv4.diffserv = 8w0;
    hdr.ipv4.total_len = hdr.inner_ipv4.total_len * (bit<16>)(bit<1>)hdr.inner_ipv4.isValid() + hdr.inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr.inner_ipv6.isValid() + 16w42;
    hdr.ipv4.identification = 16w1;
    hdr.ipv4.flags = 3w0;
    hdr.ipv4.frag_offset = 13w0;
    hdr.ipv4.ttl = 8w64;
    hdr.ipv4.protocol = 8w0x2f;
    hdr.ipv4.dst_addr = underlay_dip;
    hdr.ipv4.src_addr = underlay_sip;
    hdr.ipv4.hdr_checksum = 16w0;
    hdr.nvgre.setValid();
    hdr.nvgre.flags = 4w4;
    hdr.nvgre.reserved = 9w0;
    hdr.nvgre.version = 3w0;
    hdr.nvgre.protocol_type = 16w0x6558;
    hdr.nvgre.vsid = vsid;
    hdr.nvgre.flow_id = 8w0;
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
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", api_order=1, isobject="true"] table stage1 {
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
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", api_order=1, isobject="true"] table stage2 {
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
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", api_order=1, isobject="true"] table stage3 {
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

action service_tunnel_encode(inout headers_t hdr, in IPv6Address st_dst, in IPv6Address st_dst_mask, in IPv6Address st_src, in IPv6Address st_src_mask) {
    hdr.ipv6.setValid();
    hdr.ipv6.version = 4w6;
    hdr.ipv6.traffic_class = 8w0;
    hdr.ipv6.flow_label = 20w0;
    hdr.ipv6.payload_length = hdr.ipv4.total_len + 16w65516;
    hdr.ipv6.next_header = hdr.ipv4.protocol;
    hdr.ipv6.hop_limit = hdr.ipv4.ttl;
    hdr.ipv6.dst_addr = (IPv6Address)hdr.ipv4.dst_addr & ~st_dst_mask | st_dst & st_dst_mask;
    hdr.ipv6.src_addr = (IPv6Address)hdr.ipv4.src_addr & ~st_src_mask | st_src & st_src_mask;
    hdr.ipv4.setInvalid();
    hdr.ethernet.ether_type = 16w0x86dd;
}
action service_tunnel_decode(inout headers_t hdr, in IPv4Address src, in IPv4Address dst) {
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4w4;
    hdr.ipv4.ihl = 4w5;
    hdr.ipv4.diffserv = 8w0;
    hdr.ipv4.total_len = hdr.ipv6.payload_length + 16w20;
    hdr.ipv4.identification = 16w1;
    hdr.ipv4.flags = 3w0;
    hdr.ipv4.frag_offset = 13w0;
    hdr.ipv4.protocol = hdr.ipv6.next_header;
    hdr.ipv4.ttl = hdr.ipv6.hop_limit;
    hdr.ipv4.hdr_checksum = 16w0;
    hdr.ipv4.dst_addr = dst;
    hdr.ipv4.src_addr = src;
    hdr.ipv6.setInvalid();
    hdr.ethernet.ether_type = 16w0x800;
}
control outbound(inout headers_t hdr, inout metadata_t meta) {
    action set_route_meter_attrs(bit<1> meter_policy_en, bit<16> meter_class) {
        meta.meter_policy_en = meter_policy_en;
        meta.route_meter_class = meter_class;
    }
    action route_vnet(@SaiVal[type="sai_object_id_t"] bit<16> dst_vnet_id, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.dst_vnet_id = dst_vnet_id;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action route_vnet_direct(bit<16> dst_vnet_id, bit<1> overlay_ip_is_v6, @SaiVal[type="sai_ip_address_t"] IPv4ORv6Address overlay_ip, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.dst_vnet_id = dst_vnet_id;
        meta.lkup_dst_ip_addr = overlay_ip;
        meta.is_lkup_dst_ip_v6 = overlay_ip_is_v6;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action route_direct(bit<1> meter_policy_en, bit<16> meter_class) {
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action drop() {
        meta.dropped = true;
    }
    action route_service_tunnel(bit<1> overlay_dip_is_v6, IPv4ORv6Address overlay_dip, bit<1> overlay_dip_mask_is_v6, IPv4ORv6Address overlay_dip_mask, bit<1> overlay_sip_is_v6, IPv4ORv6Address overlay_sip, bit<1> overlay_sip_mask_is_v6, IPv4ORv6Address overlay_sip_mask, bit<1> underlay_dip_is_v6, IPv4ORv6Address underlay_dip, bit<1> underlay_sip_is_v6, IPv4ORv6Address underlay_sip, @SaiVal[type="sai_dash_encapsulation_t", default_value="SAI_DASH_ENCAPSULATION_VXLAN"] dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.encap_data.original_overlay_dip = hdr.ipv4.src_addr;
        meta.encap_data.original_overlay_sip = hdr.ipv4.dst_addr;
        service_tunnel_encode(hdr, overlay_dip, overlay_dip_mask, overlay_sip, overlay_sip_mask);
        meta.encap_data.underlay_dip = (underlay_dip == 128w0 ? meta.encap_data.original_overlay_dip : (IPv4Address)underlay_dip);
        meta.encap_data.underlay_sip = (underlay_sip == 128w0 ? meta.encap_data.original_overlay_sip : (IPv4Address)underlay_sip);
        meta.encap_data.overlay_dmac = hdr.ethernet.dst_addr;
        meta.encap_data.dash_encapsulation = dash_encapsulation;
        meta.encap_data.service_tunnel_key = tunnel_key;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    direct_counter(CounterType.packets_and_bytes) routing_counter;
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] table routing {
        key = {
            meta.eni_id          : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.is_overlay_ip_v6: exact @SaiVal[name="destination_is_v6"] @name("meta.is_overlay_ip_v6");
            meta.dst_ip_addr     : lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            route_vnet();
            route_vnet_direct();
            route_direct();
            route_service_tunnel();
            drop();
        }
        const default_action = drop();
        counters = routing_counter;
    }
    action set_tunnel(@SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, @SaiVal[type="sai_dash_encapsulation_t"] dash_encapsulation_t dash_encapsulation, bit<16> meter_class, bit<1> meter_class_override) {
        meta.encap_data.underlay_dip = underlay_dip;
        meta.mapping_meter_class = meter_class;
        meta.mapping_meter_class_override = meter_class_override;
        meta.encap_data.dash_encapsulation = dash_encapsulation;
    }
    action set_tunnel_mapping(@SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, EthernetAddress overlay_dmac, bit<1> use_dst_vnet_vni, bit<16> meter_class, bit<1> meter_class_override) {
        if (use_dst_vnet_vni == 1w1) {
            meta.vnet_id = meta.dst_vnet_id;
        }
        meta.encap_data.overlay_dmac = overlay_dmac;
        set_tunnel(underlay_dip, dash_encapsulation_t.VXLAN, meter_class, meter_class_override);
    }
    action set_private_link_mapping(@SaiVal[type="sai_ip_address_t"] IPv4Address underlay_dip, IPv6Address overlay_sip, IPv6Address overlay_dip, @SaiVal[type="sai_dash_encapsulation_t"] dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<16> meter_class, bit<1> meter_class_override) {
        meta.encap_data.overlay_dmac = hdr.ethernet.dst_addr;
        meta.encap_data.vni = tunnel_key;
        service_tunnel_encode(hdr, overlay_dip, 128w0xffffffffffffffffffffffff, overlay_sip & ~meta.eni_data.pl_sip_mask | meta.eni_data.pl_sip | (IPv6Address)hdr.ipv4.dst_addr, 128w0xffffffffffffffffffffffff);
        set_tunnel(underlay_dip, dash_encapsulation, meter_class, meter_class_override);
    }
    direct_counter(CounterType.packets_and_bytes) ca_to_pa_counter;
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] table ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @SaiVal[type="sai_object_id_t"] @name("meta.dst_vnet_id");
            meta.is_lkup_dst_ip_v6: exact @SaiVal[name="dip_is_v6"] @name("meta.is_lkup_dst_ip_v6");
            meta.lkup_dst_ip_addr : exact @SaiVal[name="dip"] @name("meta.lkup_dst_ip_addr");
        }
        actions = {
            set_tunnel_mapping();
            set_private_link_mapping();
            @defaultonly drop();
        }
        const default_action = drop();
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
    @name("acl") acl() acl_inst;
    apply {
        if (meta.conntrack_data.allow_out) {
            ;
        } else {
            acl_inst.apply(hdr, meta);
        }
        meta.lkup_dst_ip_addr = meta.dst_ip_addr;
        meta.is_lkup_dst_ip_v6 = meta.is_overlay_ip_v6;
        switch (routing.apply().action_run) {
            route_vnet_direct: 
            route_vnet: {
                switch (ca_to_pa.apply().action_run) {
                    set_tunnel_mapping: {
                        vnet.apply();
                    }
                    default: {
                    }
                }
                if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
                    vxlan_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.overlay_dmac, meta.encap_data.vni);
                } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
                    nvgre_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.overlay_dmac, meta.encap_data.vni);
                } else {
                    drop();
                }
            }
            route_service_tunnel: {
                if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
                    vxlan_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.overlay_dmac, meta.encap_data.service_tunnel_key);
                } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
                    nvgre_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.overlay_dmac, meta.encap_data.service_tunnel_key);
                } else {
                    drop();
                }
            }
            default: {
            }
        }
    }
}

control inbound(inout headers_t hdr, inout metadata_t meta) {
    @name("acl") acl() acl_inst_0;
    apply {
        if (meta.conntrack_data.allow_in) {
            ;
        } else {
            acl_inst_0.apply(hdr, meta);
        }
        vxlan_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, hdr.ethernet.dst_addr, meta.encap_data.vni);
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
    @SaiTable[name="vip", api="dash_vip"] table vip {
        key = {
            hdr.ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"] @name("hdr.ipv4.dst_addr");
        }
        actions = {
            accept();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    action set_outbound_direction() {
        meta.direction = dash_direction_t.OUTBOUND;
    }
    action set_inbound_direction() {
        meta.direction = dash_direction_t.INBOUND;
    }
    @SaiTable[name="direction_lookup", api="dash_direction_lookup"] table direction_lookup {
        key = {
            hdr.vxlan.vni: exact @SaiVal[name="VNI"] @name("hdr.vxlan.vni");
        }
        actions = {
            set_outbound_direction();
            @defaultonly set_inbound_direction();
        }
        const default_action = set_inbound_direction();
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
    action set_eni_attrs(bit<32> cps, bit<32> pps, bit<32> flows, bit<1> admin_state, @SaiVal[type="sai_ip_address_t"] IPv4Address vm_underlay_dip, @SaiVal[type="sai_uint32_t"] bit<24> vm_vni, @SaiVal[type="sai_object_id_t"] bit<16> vnet_id, IPv6Address pl_sip, IPv6Address pl_sip_mask, @SaiVal[type="sai_ip_address_t"] IPv4Address pl_underlay_sip, @SaiVal[type="sai_object_id_t"] bit<16> v4_meter_policy_id, @SaiVal[type="sai_object_id_t"] bit<16> v6_meter_policy_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> inbound_v6_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta.eni_data.cps = cps;
        meta.eni_data.pps = pps;
        meta.eni_data.flows = flows;
        meta.eni_data.admin_state = admin_state;
        meta.eni_data.pl_sip = pl_sip;
        meta.eni_data.pl_sip_mask = pl_sip_mask;
        meta.eni_data.pl_underlay_sip = pl_underlay_sip;
        meta.encap_data.underlay_dip = vm_underlay_dip;
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
    }
    @SaiTable[name="eni", api="dash_eni", api_order=1, isobject="true"] table eni {
        key = {
            meta.eni_id: exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny();
        }
        const default_action = deny();
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
    action permit() {
    }
    action vxlan_decap_pa_validate(@SaiVal[type="sai_object_id_t"] bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] table pa_validation {
        key = {
            meta.vnet_id     : exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
            hdr.ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.ipv4.src_addr");
        }
        actions = {
            permit();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] table inbound_routing {
        key = {
            meta.eni_id      : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            hdr.vxlan.vni    : exact @SaiVal[name="VNI"] @name("hdr.vxlan.vni");
            hdr.ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.ipv4.src_addr");
        }
        actions = {
            vxlan_decap(hdr);
            vxlan_decap_pa_validate();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    action check_ip_addr_family(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", api_order=1, isobject="true"] table meter_policy {
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
    @SaiTable[name="meter_rule", api="dash_meter", api_order=2, isobject="true"] table meter_rule {
        key = {
            meta.meter_policy_id: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"] @name("meta.meter_policy_id");
            hdr.ipv4.dst_addr   : ternary @SaiVal[name="dip", type="sai_ip_address_t"] @name("hdr.ipv4.dst_addr");
        }
        actions = {
            set_policy_meter_class();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
    }
    counter(32w262144, CounterType.bytes) meter_bucket_inbound;
    counter(32w262144, CounterType.bytes) meter_bucket_outbound;
    action meter_bucket_action(@SaiVal[type="sai_uint64_t", isreadonly="true"] bit<64> outbound_bytes_counter, @SaiVal[type="sai_uint64_t", isreadonly="true"] bit<64> inbound_bytes_counter, @SaiVal[type="sai_uint32_t", skipattr="true"] bit<32> meter_bucket_index) {
        meta.meter_bucket_index = meter_bucket_index;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", api_order=0, isobject="true"] table meter_bucket {
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
    action set_eni(@SaiVal[type="sai_object_id_t"] bit<16> eni_id) {
        meta.eni_id = eni_id;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", api_order=0] table eni_ether_address_map {
        key = {
            meta.eni_addr: exact @SaiVal[name="address", type="sai_mac_t"] @name("meta.eni_addr");
        }
        actions = {
            set_eni();
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
    @SaiTable[name="dash_acl_group", api="dash_acl", api_order=0, isobject="true"] table acl_group {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    @name("outbound") outbound() outbound_inst;
    @name("inbound") inbound() inbound_inst;
    @name("underlay") underlay() underlay_inst;
    apply {
        if (vip.apply().hit) {
            meta.encap_data.underlay_sip = hdr.ipv4.dst_addr;
        }
        direction_lookup.apply();
        appliance.apply();
        meta.eni_addr = (meta.direction == dash_direction_t.OUTBOUND ? hdr.inner_ethernet.src_addr : hdr.inner_ethernet.dst_addr);
        eni_ether_address_map.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            vxlan_decap(hdr);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            switch (inbound_routing.apply().action_run) {
                vxlan_decap_pa_validate: {
                    pa_validation.apply();
                    vxlan_decap(hdr);
                }
                default: {
                }
            }
        }
        meta.is_overlay_ip_v6 = 1w0;
        meta.ip_protocol = 8w0;
        meta.dst_ip_addr = 128w0;
        meta.src_ip_addr = 128w0;
        if (hdr.ipv6.isValid()) {
            meta.ip_protocol = hdr.ipv6.next_header;
            meta.src_ip_addr = hdr.ipv6.src_addr;
            meta.dst_ip_addr = hdr.ipv6.dst_addr;
            meta.is_overlay_ip_v6 = 1w1;
        } else if (hdr.ipv4.isValid()) {
            meta.ip_protocol = hdr.ipv4.protocol;
            meta.src_ip_addr = (bit<128>)hdr.ipv4.src_addr;
            meta.dst_ip_addr = (bit<128>)hdr.ipv4.dst_addr;
        }
        if (hdr.tcp.isValid()) {
            meta.src_l4_port = hdr.tcp.src_port;
            meta.dst_l4_port = hdr.tcp.dst_port;
        } else if (hdr.udp.isValid()) {
            meta.src_l4_port = hdr.udp.src_port;
            meta.dst_l4_port = hdr.udp.dst_port;
        }
        eni.apply();
        if (meta.eni_data.admin_state == 1w0) {
            deny();
        }
        acl_group.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            outbound_inst.apply(hdr, meta);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            inbound_inst.apply(hdr, meta);
        }
        meta.dst_ip_addr = (bit<128>)hdr.ipv4.dst_addr;
        underlay_inst.apply(hdr, meta, standard_metadata);
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
    }
}

control dash_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(dash_parser(), dash_verify_checksum(), dash_ingress(), dash_egress(), dash_compute_checksum(), dash_deparser()) main;
