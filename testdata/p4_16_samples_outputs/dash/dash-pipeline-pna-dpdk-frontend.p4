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

header ipv4options_t {
    varbit<320> options;
}

header udp_t {
    bit<16> src_port;
    bit<16> dst_port;
    bit<16> length;
    bit<16> checksum;
}

header vxlan_t {
    bit<8>  flags;
    bit<24> reserved;
    bit<24> vni;
    bit<8>  reserved_2;
}

header nvgre_t {
    bit<4>  flags;
    bit<9>  reserved;
    bit<3>  version;
    bit<16> protocol_type;
    bit<24> vsid;
    bit<8>  flow_id;
}

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

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
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

control dash_deparser(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
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

match_kind {
    list,
    range_list
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("dash_ingress.meta") metadata_t meta_0;
    @name("dash_ingress.meta") metadata_t meta_5;
    @name("dash_ingress.meta") metadata_t meta_6;
    @name("dash_ingress.tmp_5") bit<32> tmp;
    @name("dash_ingress.tmp_6") bit<32> tmp_0;
    @name("dash_ingress.meta") metadata_t meta_7;
    @name("dash_ingress.sip") IPv6Address sip_1;
    @name("dash_ingress.sip_mask") IPv6Address sip_mask_1;
    @name("dash_ingress.dip") IPv6Address dip_1;
    @name("dash_ingress.dip_mask") IPv6Address dip_mask_1;
    @name("dash_ingress.meta") metadata_t meta_8;
    @name("dash_ingress.encap") dash_encapsulation_t encap;
    @name("dash_ingress.vni") bit<24> vni_1;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_0;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_0;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_0;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_0;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_0;
    @name("dash_ingress.tmp") bit<24> tmp_1;
    @name("dash_ingress.tmp_0") bit<48> tmp_2;
    @name("dash_ingress.tmp_1") bit<48> tmp_3;
    @name("dash_ingress.tmp_2") bit<32> tmp_4;
    @name("dash_ingress.tmp_3") bit<32> tmp_5;
    @name("dash_ingress.tmp_4") bit<48> tmp_6;
    @name("dash_ingress.meta") metadata_t meta_9;
    @name("dash_ingress.meta") metadata_t meta_10;
    @name("dash_ingress.encap") dash_encapsulation_t encap_1;
    @name("dash_ingress.vni") bit<24> vni_3;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_1;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_1;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_1;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_1;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_1;
    @name("dash_ingress.tmp") bit<24> tmp_7;
    @name("dash_ingress.tmp_0") bit<48> tmp_8;
    @name("dash_ingress.tmp_1") bit<48> tmp_31;
    @name("dash_ingress.tmp_2") bit<32> tmp_32;
    @name("dash_ingress.tmp_3") bit<32> tmp_33;
    @name("dash_ingress.tmp_4") bit<48> tmp_34;
    @name("dash_ingress.meta") metadata_t meta_11;
    @name("dash_ingress.meter_class") bit<16> meter_class_2;
    @name("dash_ingress.meter_class_override") bit<1> meter_class_override_1;
    @name("dash_ingress.tmp_7") bit<32> tmp_35;
    @name("dash_ingress.tmp_8") bit<128> tmp_36;
    @name("dash_ingress.meta") metadata_t meta_30;
    @name("dash_ingress.encap") dash_encapsulation_t encap_2;
    @name("dash_ingress.vni") bit<24> vni_4;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_3;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_3;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_11;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_11;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_3;
    @name("dash_ingress.tmp") bit<24> tmp_37;
    @name("dash_ingress.tmp_0") bit<48> tmp_38;
    @name("dash_ingress.tmp_1") bit<48> tmp_39;
    @name("dash_ingress.tmp_2") bit<32> tmp_40;
    @name("dash_ingress.tmp_3") bit<32> tmp_41;
    @name("dash_ingress.tmp_4") bit<48> tmp_42;
    @name("dash_ingress.meta") metadata_t meta_31;
    @name("dash_ingress.sip") IPv6Address sip_2;
    @name("dash_ingress.sip_mask") IPv6Address sip_mask_2;
    @name("dash_ingress.dip") IPv6Address dip_2;
    @name("dash_ingress.dip_mask") IPv6Address dip_mask_2;
    @name("dash_ingress.meta") metadata_t meta_32;
    @name("dash_ingress.meter_class") bit<16> meter_class_3;
    @name("dash_ingress.meter_class_override") bit<1> meter_class_override_2;
    @name("dash_ingress.customer_ip_len") bit<16> customer_ip_len_0;
    @name("dash_ingress.customer_ip_len") bit<16> customer_ip_len_3;
    @name("dash_ingress.customer_ip_len") bit<16> customer_ip_len_4;
    @name("dash_ingress.u0_ip_len") bit<16> u0_ip_len_0;
    @name("dash_ingress.u0_ip_len") bit<16> u0_ip_len_3;
    @name("dash_ingress.u0_ip_len") bit<16> u0_ip_len_4;
    @name("dash_ingress.eni_lookup_stage.tmp_9") bit<48> eni_lookup_stage_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.outbound.outbound_routing_stage.hasReturned_3") bool outbound_outbound_routing_stage_hasReturned;
    @name("dash_ingress.outbound.outbound_mapping_stage.hasReturned_4") bool outbound_outbound_mapping_stage_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_nat46.hasReturned_1") bool routing_action_apply_do_action_nat46_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_nat64.hasReturned_2") bool routing_action_apply_do_action_nat64_hasReturned;
    @name("dash_ingress.routing_action_apply.do_action_static_encap.hasReturned_0") bool routing_action_apply_do_action_static_encap_hasReturned;
    @name("dash_ingress.hdr") headers_t hdr_0;
    @name("dash_ingress.meta") metadata_t meta_33;
    @name("dash_ingress.hdr") headers_t hdr_1;
    @name("dash_ingress.meta") metadata_t meta_34;
    @name("dash_ingress.hdr") headers_t hdr_2;
    @name("dash_ingress.meta") metadata_t meta_35;
    @name("dash_ingress.hdr") headers_t hdr_5;
    @name("dash_ingress.meta") metadata_t meta_36;
    @name("dash_ingress.hdr") headers_t hdr_6;
    @name("dash_ingress.meta") metadata_t meta_37;
    @name("dash_ingress.hdr") headers_t hdr_7;
    @name("dash_ingress.meta") metadata_t meta_38;
    @name("dash_ingress.hdr") headers_t hdr_8;
    @name("dash_ingress.meta") metadata_t meta_39;
    @name("dash_ingress.meta") metadata_t meta_40;
    @name("dash_ingress.meta") metadata_t meta_41;
    @name("dash_ingress.hdr") headers_t hdr_9;
    @name("dash_ingress.meta") metadata_t meta_42;
    @name("dash_ingress.hdr") headers_t hdr_10;
    @name("dash_ingress.meta") metadata_t meta_43;
    @name("dash_ingress.hdr") headers_t hdr_24;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_12;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_12;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_12;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_4;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_13;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_0;
    @name("dash_ingress.hdr") headers_t hdr_25;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_13;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_13;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_13;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_5;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_14;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_1;
    @name("dash_ingress.hdr") headers_t hdr_26;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_14;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_14;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_14;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_17;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_15;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_2;
    @name("dash_ingress.hdr") headers_t hdr_27;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_15;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_15;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_15;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_18;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_16;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_3;
    @name("dash_ingress.hdr") headers_t hdr_28;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_16;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_16;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_16;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_19;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_17;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_11;
    @name("dash_ingress.hdr") headers_t hdr_29;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_17;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_17;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_17;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_20;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_18;
    @name("dash_ingress.tunnel_key") bit<24> tunnel_key_12;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_8() {
    }
    @name(".tunnel_decap") action tunnel_decap_1() {
        hdr_0 = hdr;
        meta_33 = meta;
        hdr_0.u0_ethernet.setInvalid();
        hdr_0.u0_ipv4.setInvalid();
        hdr_0.u0_ipv6.setInvalid();
        hdr_0.u0_nvgre.setInvalid();
        hdr_0.u0_vxlan.setInvalid();
        hdr_0.u0_udp.setInvalid();
        meta_33.tunnel_pointer = 16w0;
        hdr = hdr_0;
        meta = meta_33;
    }
    @name(".tunnel_decap") action tunnel_decap_2() {
        hdr_1 = hdr;
        meta_34 = meta;
        hdr_1.u0_ethernet.setInvalid();
        hdr_1.u0_ipv4.setInvalid();
        hdr_1.u0_ipv6.setInvalid();
        hdr_1.u0_nvgre.setInvalid();
        hdr_1.u0_vxlan.setInvalid();
        hdr_1.u0_udp.setInvalid();
        meta_34.tunnel_pointer = 16w0;
        hdr = hdr_1;
        meta = meta_34;
    }
    @name(".tunnel_decap") action tunnel_decap_3() {
        hdr_2 = hdr;
        meta_35 = meta;
        hdr_2.u0_ethernet.setInvalid();
        hdr_2.u0_ipv4.setInvalid();
        hdr_2.u0_ipv6.setInvalid();
        hdr_2.u0_nvgre.setInvalid();
        hdr_2.u0_vxlan.setInvalid();
        hdr_2.u0_udp.setInvalid();
        meta_35.tunnel_pointer = 16w0;
        hdr = hdr_2;
        meta = meta_35;
    }
    @name(".route_vnet") action route_vnet_0(@SaiVal[type="sai_object_id_t"] @name("dst_vnet_id") bit<16> dst_vnet_id_2, @name("meter_policy_en") bit<1> meter_policy_en_0, @name("meter_class") bit<16> meter_class_9) {
        hdr_5 = hdr;
        meta_36 = meta;
        meta_36.target_stage = dash_pipeline_stage_t.OUTBOUND_MAPPING;
        meta_36.dst_vnet_id = dst_vnet_id_2;
        meta_0 = meta_36;
        meta_0.meter_policy_en = meter_policy_en_0;
        meta_0.route_meter_class = meter_class_9;
        meta_36 = meta_0;
        hdr = hdr_5;
        meta = meta_36;
    }
    @name(".route_vnet_direct") action route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("overlay_ip_is_v6") bit<1> overlay_ip_is_v6, @SaiVal[type="sai_ip_address_t"] @name("overlay_ip") IPv4ORv6Address overlay_ip, @name("meter_policy_en") bit<1> meter_policy_en_5, @name("meter_class") bit<16> meter_class_10) {
        hdr_6 = hdr;
        meta_37 = meta;
        meta_37.target_stage = dash_pipeline_stage_t.OUTBOUND_MAPPING;
        meta_37.dst_vnet_id = dst_vnet_id_3;
        meta_37.lkup_dst_ip_addr = overlay_ip;
        meta_37.is_lkup_dst_ip_v6 = overlay_ip_is_v6;
        meta_5 = meta_37;
        meta_5.meter_policy_en = meter_policy_en_5;
        meta_5.route_meter_class = meter_class_10;
        meta_37 = meta_5;
        hdr = hdr_6;
        meta = meta_37;
    }
    @name(".route_direct") action route_direct_0(@name("meter_policy_en") bit<1> meter_policy_en_6, @name("meter_class") bit<16> meter_class_11) {
        hdr_7 = hdr;
        meta_38 = meta;
        meta_38.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        meta_6 = meta_38;
        meta_6.meter_policy_en = meter_policy_en_6;
        meta_6.route_meter_class = meter_class_11;
        meta_38 = meta_6;
        hdr = hdr_7;
        meta = meta_38;
    }
    @name(".route_service_tunnel") action route_service_tunnel_0(@name("overlay_dip_is_v6") bit<1> overlay_dip_is_v6, @name("overlay_dip") IPv4ORv6Address overlay_dip, @name("overlay_dip_mask_is_v6") bit<1> overlay_dip_mask_is_v6, @name("overlay_dip_mask") IPv4ORv6Address overlay_dip_mask, @name("overlay_sip_is_v6") bit<1> overlay_sip_is_v6, @name("overlay_sip") IPv4ORv6Address overlay_sip, @name("overlay_sip_mask_is_v6") bit<1> overlay_sip_mask_is_v6, @name("overlay_sip_mask") IPv4ORv6Address overlay_sip_mask, @name("underlay_dip_is_v6") bit<1> underlay_dip_is_v6, @name("underlay_dip") IPv4ORv6Address underlay_dip_6, @name("underlay_sip_is_v6") bit<1> underlay_sip_is_v6, @name("underlay_sip") IPv4ORv6Address underlay_sip_4, @SaiVal[type="sai_dash_encapsulation_t", default_value="SAI_DASH_ENCAPSULATION_VXLAN"] @name("dash_encapsulation") dash_encapsulation_t dash_encapsulation_2, @name("tunnel_key") bit<24> tunnel_key, @name("meter_policy_en") bit<1> meter_policy_en_7, @name("meter_class") bit<16> meter_class_12) {
        hdr_8 = hdr;
        meta_39 = meta;
        meta_39.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        meta_7 = meta_39;
        sip_1 = overlay_sip;
        sip_mask_1 = overlay_sip_mask;
        dip_1 = overlay_dip;
        dip_mask_1 = overlay_dip_mask;
        meta_7.routing_actions = meta_7.routing_actions | 32w2;
        meta_7.overlay_data.is_ipv6 = true;
        meta_7.overlay_data.sip = sip_1;
        meta_7.overlay_data.sip_mask = sip_mask_1;
        meta_7.overlay_data.dip = dip_1;
        meta_7.overlay_data.dip_mask = dip_mask_1;
        meta_39 = meta_7;
        if (underlay_sip_4 == 128w0) {
            tmp = hdr_8.u0_ipv4.src_addr;
        } else {
            tmp = (IPv4Address)underlay_sip_4;
        }
        if (underlay_dip_6 == 128w0) {
            tmp_0 = hdr_8.u0_ipv4.dst_addr;
        } else {
            tmp_0 = (IPv4Address)underlay_dip_6;
        }
        meta_8 = meta_39;
        encap = dash_encapsulation_2;
        vni_1 = tunnel_key;
        underlay_sip_0 = tmp;
        underlay_dip_0 = tmp_0;
        underlay_smac_0 = 48w0;
        underlay_dmac_0 = 48w0;
        overlay_dmac_0 = hdr_8.u0_ethernet.dst_addr;
        meta_8.routing_actions = meta_8.routing_actions | 32w1;
        meta_8.encap_data.dash_encapsulation = encap;
        if (vni_1 == 24w0) {
            tmp_1 = meta_8.encap_data.vni;
        } else {
            tmp_1 = vni_1;
        }
        meta_8.encap_data.vni = tmp_1;
        if (underlay_smac_0 == 48w0) {
            tmp_2 = meta_8.encap_data.underlay_smac;
        } else {
            tmp_2 = underlay_smac_0;
        }
        meta_8.encap_data.underlay_smac = tmp_2;
        if (underlay_dmac_0 == 48w0) {
            tmp_3 = meta_8.encap_data.underlay_dmac;
        } else {
            tmp_3 = underlay_dmac_0;
        }
        meta_8.encap_data.underlay_dmac = tmp_3;
        if (underlay_sip_0 == 32w0) {
            tmp_4 = meta_8.encap_data.underlay_sip;
        } else {
            tmp_4 = underlay_sip_0;
        }
        meta_8.encap_data.underlay_sip = tmp_4;
        if (underlay_dip_0 == 32w0) {
            tmp_5 = meta_8.encap_data.underlay_dip;
        } else {
            tmp_5 = underlay_dip_0;
        }
        meta_8.encap_data.underlay_dip = tmp_5;
        if (overlay_dmac_0 == 48w0) {
            tmp_6 = meta_8.overlay_data.dmac;
        } else {
            tmp_6 = overlay_dmac_0;
        }
        meta_8.overlay_data.dmac = tmp_6;
        meta_39 = meta_8;
        meta_9 = meta_39;
        meta_9.meter_policy_en = meter_policy_en_7;
        meta_9.route_meter_class = meter_class_12;
        meta_39 = meta_9;
        hdr = hdr_8;
        meta = meta_39;
    }
    @name(".drop") action drop_1() {
        meta_40 = meta;
        meta_40.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        meta_40.dropped = true;
        meta = meta_40;
    }
    @name(".drop") action drop_2() {
        meta_41 = meta;
        meta_41.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        meta_41.dropped = true;
        meta = meta_41;
    }
    @name(".set_tunnel_mapping") action set_tunnel_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") IPv4Address underlay_dip_7, @name("overlay_dmac") EthernetAddress overlay_dmac, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni, @name("meter_class") bit<16> meter_class_13, @name("meter_class_override") bit<1> meter_class_override) {
        hdr_9 = hdr;
        meta_42 = meta;
        meta_42.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        if (use_dst_vnet_vni == 1w1) {
            meta_42.vnet_id = meta_42.dst_vnet_id;
        }
        meta_10 = meta_42;
        encap_1 = dash_encapsulation_t.VXLAN;
        vni_3 = 24w0;
        underlay_sip_1 = 32w0;
        underlay_dip_1 = underlay_dip_7;
        underlay_smac_1 = 48w0;
        underlay_dmac_1 = 48w0;
        overlay_dmac_1 = overlay_dmac;
        meta_10.routing_actions = meta_10.routing_actions | 32w1;
        meta_10.encap_data.dash_encapsulation = encap_1;
        if (vni_3 == 24w0) {
            tmp_7 = meta_10.encap_data.vni;
        } else {
            tmp_7 = vni_3;
        }
        meta_10.encap_data.vni = tmp_7;
        if (underlay_smac_1 == 48w0) {
            tmp_8 = meta_10.encap_data.underlay_smac;
        } else {
            tmp_8 = underlay_smac_1;
        }
        meta_10.encap_data.underlay_smac = tmp_8;
        if (underlay_dmac_1 == 48w0) {
            tmp_31 = meta_10.encap_data.underlay_dmac;
        } else {
            tmp_31 = underlay_dmac_1;
        }
        meta_10.encap_data.underlay_dmac = tmp_31;
        if (underlay_sip_1 == 32w0) {
            tmp_32 = meta_10.encap_data.underlay_sip;
        } else {
            tmp_32 = underlay_sip_1;
        }
        meta_10.encap_data.underlay_sip = tmp_32;
        if (underlay_dip_1 == 32w0) {
            tmp_33 = meta_10.encap_data.underlay_dip;
        } else {
            tmp_33 = underlay_dip_1;
        }
        meta_10.encap_data.underlay_dip = tmp_33;
        if (overlay_dmac_1 == 48w0) {
            tmp_34 = meta_10.overlay_data.dmac;
        } else {
            tmp_34 = overlay_dmac_1;
        }
        meta_10.overlay_data.dmac = tmp_34;
        meta_42 = meta_10;
        meta_11 = meta_42;
        meter_class_2 = meter_class_13;
        meter_class_override_1 = meter_class_override;
        meta_11.mapping_meter_class = meter_class_2;
        meta_11.mapping_meter_class_override = meter_class_override_1;
        meta_42 = meta_11;
        hdr = hdr_9;
        meta = meta_42;
    }
    @name(".set_private_link_mapping") action set_private_link_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") IPv4Address underlay_dip_8, @name("overlay_sip") IPv6Address overlay_sip_2, @name("overlay_dip") IPv6Address overlay_dip_2, @SaiVal[type="sai_dash_encapsulation_t"] @name("dash_encapsulation") dash_encapsulation_t dash_encapsulation_3, @name("tunnel_key") bit<24> tunnel_key_4, @name("meter_class") bit<16> meter_class_14, @name("meter_class_override") bit<1> meter_class_override_3) {
        hdr_10 = hdr;
        meta_43 = meta;
        meta_43.target_stage = dash_pipeline_stage_t.ROUTING_ACTION_APPLY;
        tmp_35 = meta_43.eni_data.pl_underlay_sip;
        meta_30 = meta_43;
        encap_2 = dash_encapsulation_3;
        vni_4 = tunnel_key_4;
        underlay_sip_3 = tmp_35;
        underlay_dip_3 = underlay_dip_8;
        underlay_smac_11 = 48w0;
        underlay_dmac_11 = 48w0;
        overlay_dmac_3 = hdr_10.u0_ethernet.dst_addr;
        meta_30.routing_actions = meta_30.routing_actions | 32w1;
        meta_30.encap_data.dash_encapsulation = encap_2;
        if (vni_4 == 24w0) {
            tmp_37 = meta_30.encap_data.vni;
        } else {
            tmp_37 = vni_4;
        }
        meta_30.encap_data.vni = tmp_37;
        if (underlay_smac_11 == 48w0) {
            tmp_38 = meta_30.encap_data.underlay_smac;
        } else {
            tmp_38 = underlay_smac_11;
        }
        meta_30.encap_data.underlay_smac = tmp_38;
        if (underlay_dmac_11 == 48w0) {
            tmp_39 = meta_30.encap_data.underlay_dmac;
        } else {
            tmp_39 = underlay_dmac_11;
        }
        meta_30.encap_data.underlay_dmac = tmp_39;
        if (underlay_sip_3 == 32w0) {
            tmp_40 = meta_30.encap_data.underlay_sip;
        } else {
            tmp_40 = underlay_sip_3;
        }
        meta_30.encap_data.underlay_sip = tmp_40;
        if (underlay_dip_3 == 32w0) {
            tmp_41 = meta_30.encap_data.underlay_dip;
        } else {
            tmp_41 = underlay_dip_3;
        }
        meta_30.encap_data.underlay_dip = tmp_41;
        if (overlay_dmac_3 == 48w0) {
            tmp_42 = meta_30.overlay_data.dmac;
        } else {
            tmp_42 = overlay_dmac_3;
        }
        meta_30.overlay_data.dmac = tmp_42;
        meta_43 = meta_30;
        tmp_36 = overlay_sip_2 & ~meta_43.eni_data.pl_sip_mask | meta_43.eni_data.pl_sip | (IPv6Address)hdr_10.u0_ipv4.src_addr;
        meta_31 = meta_43;
        sip_2 = tmp_36;
        sip_mask_2 = 128w0xffffffffffffffffffffffff;
        dip_2 = overlay_dip_2;
        dip_mask_2 = 128w0xffffffffffffffffffffffff;
        meta_31.routing_actions = meta_31.routing_actions | 32w2;
        meta_31.overlay_data.is_ipv6 = true;
        meta_31.overlay_data.sip = sip_2;
        meta_31.overlay_data.sip_mask = sip_mask_2;
        meta_31.overlay_data.dip = dip_2;
        meta_31.overlay_data.dip_mask = dip_mask_2;
        meta_43 = meta_31;
        meta_32 = meta_43;
        meter_class_3 = meter_class_14;
        meter_class_override_2 = meter_class_override_3;
        meta_32.mapping_meter_class = meter_class_3;
        meta_32.mapping_meter_class_override = meter_class_override_2;
        meta_43 = meta_32;
        hdr = hdr_10;
        meta = meta_43;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_1() {
        hdr_24 = hdr;
        overlay_dmac_12 = meta.overlay_data.dmac;
        underlay_dmac_12 = meta.encap_data.underlay_dmac;
        underlay_smac_12 = meta.encap_data.underlay_smac;
        underlay_dip_4 = meta.encap_data.underlay_dip;
        underlay_sip_13 = meta.encap_data.underlay_sip;
        tunnel_key_0 = meta.encap_data.vni;
        hdr_24.customer_ethernet.dst_addr = overlay_dmac_12;
        hdr_24.u0_ethernet.setValid();
        hdr_24.u0_ethernet.dst_addr = underlay_dmac_12;
        hdr_24.u0_ethernet.src_addr = underlay_smac_12;
        hdr_24.u0_ethernet.ether_type = 16w0x800;
        hdr_24.u0_ipv4.setValid();
        customer_ip_len_0 = 16w0;
        if (hdr_24.customer_ipv4.isValid()) {
            customer_ip_len_0 = customer_ip_len_0 + hdr_24.customer_ipv4.total_len;
        }
        if (hdr_24.customer_ipv6.isValid()) {
            customer_ip_len_0 = customer_ip_len_0 + 16w40 + hdr_24.customer_ipv6.payload_length;
        }
        hdr_24.u0_ipv4.total_len = 16w50 + customer_ip_len_0;
        hdr_24.u0_ipv4.version = 4w4;
        hdr_24.u0_ipv4.ihl = 4w5;
        hdr_24.u0_ipv4.diffserv = 8w0;
        hdr_24.u0_ipv4.identification = 16w1;
        hdr_24.u0_ipv4.flags = 3w0;
        hdr_24.u0_ipv4.frag_offset = 13w0;
        hdr_24.u0_ipv4.ttl = 8w64;
        hdr_24.u0_ipv4.protocol = 8w17;
        hdr_24.u0_ipv4.dst_addr = underlay_dip_4;
        hdr_24.u0_ipv4.src_addr = underlay_sip_13;
        hdr_24.u0_ipv4.hdr_checksum = 16w0;
        hdr_24.u0_udp.setValid();
        hdr_24.u0_udp.src_port = 16w0;
        hdr_24.u0_udp.dst_port = 16w4789;
        hdr_24.u0_udp.length = 16w30 + customer_ip_len_0;
        hdr_24.u0_udp.checksum = 16w0;
        hdr_24.u0_vxlan.setValid();
        hdr_24.u0_vxlan.reserved = 24w0;
        hdr_24.u0_vxlan.reserved_2 = 8w0;
        hdr_24.u0_vxlan.flags = 8w0;
        hdr_24.u0_vxlan.vni = tunnel_key_0;
        hdr = hdr_24;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_2() {
        hdr_25 = hdr;
        overlay_dmac_13 = meta.overlay_data.dmac;
        underlay_dmac_13 = meta.encap_data.underlay_dmac;
        underlay_smac_13 = meta.encap_data.underlay_smac;
        underlay_dip_5 = meta.encap_data.underlay_dip;
        underlay_sip_14 = meta.encap_data.underlay_sip;
        tunnel_key_1 = meta.encap_data.vni;
        hdr_25.customer_ethernet.dst_addr = overlay_dmac_13;
        hdr_25.u0_ethernet.setValid();
        hdr_25.u0_ethernet.dst_addr = underlay_dmac_13;
        hdr_25.u0_ethernet.src_addr = underlay_smac_13;
        hdr_25.u0_ethernet.ether_type = 16w0x800;
        hdr_25.u0_ipv4.setValid();
        customer_ip_len_3 = 16w0;
        if (hdr_25.customer_ipv4.isValid()) {
            customer_ip_len_3 = customer_ip_len_3 + hdr_25.customer_ipv4.total_len;
        }
        if (hdr_25.customer_ipv6.isValid()) {
            customer_ip_len_3 = customer_ip_len_3 + 16w40 + hdr_25.customer_ipv6.payload_length;
        }
        hdr_25.u0_ipv4.total_len = 16w50 + customer_ip_len_3;
        hdr_25.u0_ipv4.version = 4w4;
        hdr_25.u0_ipv4.ihl = 4w5;
        hdr_25.u0_ipv4.diffserv = 8w0;
        hdr_25.u0_ipv4.identification = 16w1;
        hdr_25.u0_ipv4.flags = 3w0;
        hdr_25.u0_ipv4.frag_offset = 13w0;
        hdr_25.u0_ipv4.ttl = 8w64;
        hdr_25.u0_ipv4.protocol = 8w17;
        hdr_25.u0_ipv4.dst_addr = underlay_dip_5;
        hdr_25.u0_ipv4.src_addr = underlay_sip_14;
        hdr_25.u0_ipv4.hdr_checksum = 16w0;
        hdr_25.u0_udp.setValid();
        hdr_25.u0_udp.src_port = 16w0;
        hdr_25.u0_udp.dst_port = 16w4789;
        hdr_25.u0_udp.length = 16w30 + customer_ip_len_3;
        hdr_25.u0_udp.checksum = 16w0;
        hdr_25.u0_vxlan.setValid();
        hdr_25.u0_vxlan.reserved = 24w0;
        hdr_25.u0_vxlan.reserved_2 = 8w0;
        hdr_25.u0_vxlan.flags = 8w0;
        hdr_25.u0_vxlan.vni = tunnel_key_1;
        hdr = hdr_25;
    }
    @name(".push_vxlan_tunnel_u0") action push_vxlan_tunnel_u0_3() {
        hdr_26 = hdr;
        overlay_dmac_14 = meta.overlay_data.dmac;
        underlay_dmac_14 = meta.encap_data.underlay_dmac;
        underlay_smac_14 = meta.encap_data.underlay_smac;
        underlay_dip_17 = meta.encap_data.underlay_dip;
        underlay_sip_15 = meta.encap_data.underlay_sip;
        tunnel_key_2 = meta.encap_data.vni;
        hdr_26.customer_ethernet.dst_addr = overlay_dmac_14;
        hdr_26.u0_ethernet.setValid();
        hdr_26.u0_ethernet.dst_addr = underlay_dmac_14;
        hdr_26.u0_ethernet.src_addr = underlay_smac_14;
        hdr_26.u0_ethernet.ether_type = 16w0x800;
        hdr_26.u0_ipv4.setValid();
        customer_ip_len_4 = 16w0;
        if (hdr_26.customer_ipv4.isValid()) {
            customer_ip_len_4 = customer_ip_len_4 + hdr_26.customer_ipv4.total_len;
        }
        if (hdr_26.customer_ipv6.isValid()) {
            customer_ip_len_4 = customer_ip_len_4 + 16w40 + hdr_26.customer_ipv6.payload_length;
        }
        hdr_26.u0_ipv4.total_len = 16w50 + customer_ip_len_4;
        hdr_26.u0_ipv4.version = 4w4;
        hdr_26.u0_ipv4.ihl = 4w5;
        hdr_26.u0_ipv4.diffserv = 8w0;
        hdr_26.u0_ipv4.identification = 16w1;
        hdr_26.u0_ipv4.flags = 3w0;
        hdr_26.u0_ipv4.frag_offset = 13w0;
        hdr_26.u0_ipv4.ttl = 8w64;
        hdr_26.u0_ipv4.protocol = 8w17;
        hdr_26.u0_ipv4.dst_addr = underlay_dip_17;
        hdr_26.u0_ipv4.src_addr = underlay_sip_15;
        hdr_26.u0_ipv4.hdr_checksum = 16w0;
        hdr_26.u0_udp.setValid();
        hdr_26.u0_udp.src_port = 16w0;
        hdr_26.u0_udp.dst_port = 16w4789;
        hdr_26.u0_udp.length = 16w30 + customer_ip_len_4;
        hdr_26.u0_udp.checksum = 16w0;
        hdr_26.u0_vxlan.setValid();
        hdr_26.u0_vxlan.reserved = 24w0;
        hdr_26.u0_vxlan.reserved_2 = 8w0;
        hdr_26.u0_vxlan.flags = 8w0;
        hdr_26.u0_vxlan.vni = tunnel_key_2;
        hdr = hdr_26;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_1() {
        hdr_27 = hdr;
        overlay_dmac_15 = meta.overlay_data.dmac;
        underlay_dmac_15 = meta.encap_data.underlay_dmac;
        underlay_smac_15 = meta.encap_data.underlay_smac;
        underlay_dip_18 = meta.encap_data.underlay_dip;
        underlay_sip_16 = meta.encap_data.underlay_sip;
        tunnel_key_3 = meta.encap_data.vni;
        hdr_27.u0_ethernet.dst_addr = overlay_dmac_15;
        hdr_27.u1_ethernet.setValid();
        hdr_27.u1_ethernet.dst_addr = underlay_dmac_15;
        hdr_27.u1_ethernet.src_addr = underlay_smac_15;
        hdr_27.u1_ethernet.ether_type = 16w0x800;
        hdr_27.u1_ipv4.setValid();
        u0_ip_len_0 = 16w0;
        if (hdr_27.u0_ipv4.isValid()) {
            u0_ip_len_0 = u0_ip_len_0 + hdr_27.u0_ipv4.total_len;
        }
        if (hdr_27.u0_ipv6.isValid()) {
            u0_ip_len_0 = u0_ip_len_0 + 16w40 + hdr_27.u0_ipv6.payload_length;
        }
        hdr_27.u1_ipv4.total_len = 16w50 + u0_ip_len_0;
        hdr_27.u1_ipv4.version = 4w4;
        hdr_27.u1_ipv4.ihl = 4w5;
        hdr_27.u1_ipv4.diffserv = 8w0;
        hdr_27.u1_ipv4.identification = 16w1;
        hdr_27.u1_ipv4.flags = 3w0;
        hdr_27.u1_ipv4.frag_offset = 13w0;
        hdr_27.u1_ipv4.ttl = 8w64;
        hdr_27.u1_ipv4.protocol = 8w17;
        hdr_27.u1_ipv4.dst_addr = underlay_dip_18;
        hdr_27.u1_ipv4.src_addr = underlay_sip_16;
        hdr_27.u1_ipv4.hdr_checksum = 16w0;
        hdr_27.u1_udp.setValid();
        hdr_27.u1_udp.src_port = 16w0;
        hdr_27.u1_udp.dst_port = 16w4789;
        hdr_27.u1_udp.length = 16w30 + u0_ip_len_0;
        hdr_27.u1_udp.checksum = 16w0;
        hdr_27.u1_vxlan.setValid();
        hdr_27.u1_vxlan.reserved = 24w0;
        hdr_27.u1_vxlan.reserved_2 = 8w0;
        hdr_27.u1_vxlan.flags = 8w0;
        hdr_27.u1_vxlan.vni = tunnel_key_3;
        hdr = hdr_27;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_2() {
        hdr_28 = hdr;
        overlay_dmac_16 = meta.overlay_data.dmac;
        underlay_dmac_16 = meta.encap_data.underlay_dmac;
        underlay_smac_16 = meta.encap_data.underlay_smac;
        underlay_dip_19 = meta.encap_data.underlay_dip;
        underlay_sip_17 = meta.encap_data.underlay_sip;
        tunnel_key_11 = meta.encap_data.vni;
        hdr_28.u0_ethernet.dst_addr = overlay_dmac_16;
        hdr_28.u1_ethernet.setValid();
        hdr_28.u1_ethernet.dst_addr = underlay_dmac_16;
        hdr_28.u1_ethernet.src_addr = underlay_smac_16;
        hdr_28.u1_ethernet.ether_type = 16w0x800;
        hdr_28.u1_ipv4.setValid();
        u0_ip_len_3 = 16w0;
        if (hdr_28.u0_ipv4.isValid()) {
            u0_ip_len_3 = u0_ip_len_3 + hdr_28.u0_ipv4.total_len;
        }
        if (hdr_28.u0_ipv6.isValid()) {
            u0_ip_len_3 = u0_ip_len_3 + 16w40 + hdr_28.u0_ipv6.payload_length;
        }
        hdr_28.u1_ipv4.total_len = 16w50 + u0_ip_len_3;
        hdr_28.u1_ipv4.version = 4w4;
        hdr_28.u1_ipv4.ihl = 4w5;
        hdr_28.u1_ipv4.diffserv = 8w0;
        hdr_28.u1_ipv4.identification = 16w1;
        hdr_28.u1_ipv4.flags = 3w0;
        hdr_28.u1_ipv4.frag_offset = 13w0;
        hdr_28.u1_ipv4.ttl = 8w64;
        hdr_28.u1_ipv4.protocol = 8w17;
        hdr_28.u1_ipv4.dst_addr = underlay_dip_19;
        hdr_28.u1_ipv4.src_addr = underlay_sip_17;
        hdr_28.u1_ipv4.hdr_checksum = 16w0;
        hdr_28.u1_udp.setValid();
        hdr_28.u1_udp.src_port = 16w0;
        hdr_28.u1_udp.dst_port = 16w4789;
        hdr_28.u1_udp.length = 16w30 + u0_ip_len_3;
        hdr_28.u1_udp.checksum = 16w0;
        hdr_28.u1_vxlan.setValid();
        hdr_28.u1_vxlan.reserved = 24w0;
        hdr_28.u1_vxlan.reserved_2 = 8w0;
        hdr_28.u1_vxlan.flags = 8w0;
        hdr_28.u1_vxlan.vni = tunnel_key_11;
        hdr = hdr_28;
    }
    @name(".push_vxlan_tunnel_u1") action push_vxlan_tunnel_u1_3() {
        hdr_29 = hdr;
        overlay_dmac_17 = meta.overlay_data.dmac;
        underlay_dmac_17 = meta.encap_data.underlay_dmac;
        underlay_smac_17 = meta.encap_data.underlay_smac;
        underlay_dip_20 = meta.encap_data.underlay_dip;
        underlay_sip_18 = meta.encap_data.underlay_sip;
        tunnel_key_12 = meta.encap_data.vni;
        hdr_29.u0_ethernet.dst_addr = overlay_dmac_17;
        hdr_29.u1_ethernet.setValid();
        hdr_29.u1_ethernet.dst_addr = underlay_dmac_17;
        hdr_29.u1_ethernet.src_addr = underlay_smac_17;
        hdr_29.u1_ethernet.ether_type = 16w0x800;
        hdr_29.u1_ipv4.setValid();
        u0_ip_len_4 = 16w0;
        if (hdr_29.u0_ipv4.isValid()) {
            u0_ip_len_4 = u0_ip_len_4 + hdr_29.u0_ipv4.total_len;
        }
        if (hdr_29.u0_ipv6.isValid()) {
            u0_ip_len_4 = u0_ip_len_4 + 16w40 + hdr_29.u0_ipv6.payload_length;
        }
        hdr_29.u1_ipv4.total_len = 16w50 + u0_ip_len_4;
        hdr_29.u1_ipv4.version = 4w4;
        hdr_29.u1_ipv4.ihl = 4w5;
        hdr_29.u1_ipv4.diffserv = 8w0;
        hdr_29.u1_ipv4.identification = 16w1;
        hdr_29.u1_ipv4.flags = 3w0;
        hdr_29.u1_ipv4.frag_offset = 13w0;
        hdr_29.u1_ipv4.ttl = 8w64;
        hdr_29.u1_ipv4.protocol = 8w17;
        hdr_29.u1_ipv4.dst_addr = underlay_dip_20;
        hdr_29.u1_ipv4.src_addr = underlay_sip_18;
        hdr_29.u1_ipv4.hdr_checksum = 16w0;
        hdr_29.u1_udp.setValid();
        hdr_29.u1_udp.src_port = 16w0;
        hdr_29.u1_udp.dst_port = 16w4789;
        hdr_29.u1_udp.length = 16w30 + u0_ip_len_4;
        hdr_29.u1_udp.checksum = 16w0;
        hdr_29.u1_vxlan.setValid();
        hdr_29.u1_vxlan.reserved = 24w0;
        hdr_29.u1_vxlan.reserved_2 = 8w0;
        hdr_29.u1_vxlan.flags = 8w0;
        hdr_29.u1_vxlan.vni = tunnel_key_12;
        hdr = hdr_29;
    }
    @name("dash_ingress.drop_action") action drop_action() {
        drop_packet();
    }
    @name("dash_ingress.deny") action deny() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_3() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_4() {
        meta.dropped = true;
    }
    @name("dash_ingress.accept") action accept_1() {
    }
    @SaiTable[name="vip", api="dash_vip"] @name("dash_ingress.vip") table vip_0 {
        key = {
            hdr.u0_ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            accept_1();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @name("dash_ingress.set_appliance") action set_appliance(@name("neighbor_mac") EthernetAddress neighbor_mac, @name("mac") EthernetAddress mac) {
        meta.encap_data.underlay_dmac = neighbor_mac;
        meta.encap_data.underlay_smac = mac;
    }
    @SaiTable[ignored="true"] @name("dash_ingress.appliance") table appliance_0 {
        key = {
            meta.appliance_id: ternary @name("meta.appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @SaiVal[type="sai_ip_address_t"] @name("vm_underlay_dip") IPv4Address vm_underlay_dip, @SaiVal[type="sai_uint32_t"] @name("vm_vni") bit<24> vm_vni, @SaiVal[type="sai_object_id_t"] @name("vnet_id") bit<16> vnet_id_1, @name("pl_sip") IPv6Address pl_sip_1, @name("pl_sip_mask") IPv6Address pl_sip_mask_1, @SaiVal[type="sai_ip_address_t"] @name("pl_underlay_sip") IPv4Address pl_underlay_sip_1, @SaiVal[type="sai_object_id_t"] @name("v4_meter_policy_id") bit<16> v4_meter_policy_id, @SaiVal[type="sai_object_id_t"] @name("v6_meter_policy_id") bit<16> v6_meter_policy_id, @SaiVal[type="sai_dash_tunnel_dscp_mode_t"] @name("dash_tunnel_dscp_mode") dash_tunnel_dscp_mode_t dash_tunnel_dscp_mode, @SaiVal[type="sai_uint8_t", validonly="SAI_ENI_ATTR_DASH_TUNNEL_DSCP_MODE == SAI_DASH_TUNNEL_DSCP_MODE_PIPE_MODEL"] @name("dscp") bit<6> dscp_1, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id, @name("disable_fast_path_icmp_flow_redirection") bit<1> disable_fast_path_icmp_flow_redirection) {
        meta.eni_data.cps = cps_1;
        meta.eni_data.pps = pps_1;
        meta.eni_data.flows = flows_1;
        meta.eni_data.admin_state = admin_state_1;
        meta.eni_data.pl_sip = pl_sip_1;
        meta.eni_data.pl_sip_mask = pl_sip_mask_1;
        meta.eni_data.pl_underlay_sip = pl_underlay_sip_1;
        meta.encap_data.underlay_dip = vm_underlay_dip;
        if (dash_tunnel_dscp_mode == dash_tunnel_dscp_mode_t.PIPE_MODEL) {
            meta.eni_data.dscp = dscp_1;
        }
        meta.encap_data.vni = vm_vni;
        meta.vnet_id = vnet_id_1;
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
    @SaiTable[name="eni", api="dash_eni", order=1, isobject="true"] @name("dash_ingress.eni") table eni_0 {
        key = {
            meta.eni_id: exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny_0();
        }
        const default_action = deny_0();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.tunnel_decap_pa_validate") action tunnel_decap_pa_validate(@SaiVal[type="sai_object_id_t"] @name("src_vnet_id") bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] @name("dash_ingress.pa_validation") table pa_validation_0 {
        key = {
            meta.vnet_id        : exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
            hdr.u0_ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            permit();
            @defaultonly deny_1();
        }
        const default_action = deny_1();
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] @name("dash_ingress.inbound_routing") table inbound_routing_0 {
        key = {
            meta.eni_id         : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            hdr.u0_vxlan.vni    : exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
            hdr.u0_ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.src_addr");
        }
        actions = {
            tunnel_decap_1();
            tunnel_decap_pa_validate();
            @defaultonly deny_3();
        }
        const default_action = deny_3();
    }
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @SaiTable[name="dash_acl_group", api="dash_acl", order=0, isobject="true"] @name("dash_ingress.acl_group") table acl_group_0 {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("dash_ingress.direction_lookup_stage.set_outbound_direction") action direction_lookup_stage_set_outbound_direction_0() {
        meta.direction = dash_direction_t.OUTBOUND;
    }
    @name("dash_ingress.direction_lookup_stage.set_inbound_direction") action direction_lookup_stage_set_inbound_direction_0() {
        meta.direction = dash_direction_t.INBOUND;
    }
    @SaiTable[name="direction_lookup", api="dash_direction_lookup"] @name("dash_ingress.direction_lookup_stage.direction_lookup") table direction_lookup_stage_direction_lookup {
        key = {
            hdr.u0_vxlan.vni: exact @SaiVal[name="VNI"] @name("hdr.u0_vxlan.vni");
        }
        actions = {
            direction_lookup_stage_set_outbound_direction_0();
            @defaultonly direction_lookup_stage_set_inbound_direction_0();
        }
        const default_action = direction_lookup_stage_set_inbound_direction_0();
    }
    @name("dash_ingress.eni_lookup_stage.set_eni") action eni_lookup_stage_set_eni_0(@SaiVal[type="sai_object_id_t"] @name("eni_id") bit<16> eni_id_1) {
        meta.eni_id = eni_id_1;
    }
    @name("dash_ingress.eni_lookup_stage.deny") action eni_lookup_stage_deny_0() {
        meta.dropped = true;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", order=0] @name("dash_ingress.eni_lookup_stage.eni_ether_address_map") table eni_lookup_stage_eni_ether_address_map {
        key = {
            meta.eni_addr: exact @SaiVal[name="address", type="sai_mac_t"] @name("meta.eni_addr");
        }
        actions = {
            eni_lookup_stage_set_eni_0();
            @defaultonly eni_lookup_stage_deny_0();
        }
        const default_action = eni_lookup_stage_deny_0();
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_0() {
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_1() {
    }
    @name("dash_ingress.outbound.acl.permit") action outbound_acl_permit_2() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_0() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_1() {
    }
    @name("dash_ingress.outbound.acl.permit_and_continue") action outbound_acl_permit_and_continue_2() {
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_2() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_2() {
        meta.dropped = true;
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", isobject="true"] @name("dash_ingress.outbound.acl.stage1") table outbound_acl_stage1 {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_0();
            outbound_acl_permit_and_continue_0();
            outbound_acl_deny_0();
            outbound_acl_deny_and_continue_0();
        }
        default_action = outbound_acl_deny_0();
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", isobject="true"] @name("dash_ingress.outbound.acl.stage2") table outbound_acl_stage2 {
        key = {
            meta.stage2_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage2_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_1();
            outbound_acl_permit_and_continue_1();
            outbound_acl_deny_1();
            outbound_acl_deny_and_continue_1();
        }
        default_action = outbound_acl_deny_1();
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", isobject="true"] @name("dash_ingress.outbound.acl.stage3") table outbound_acl_stage3 {
        key = {
            meta.stage3_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage3_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_2();
            outbound_acl_permit_and_continue_2();
            outbound_acl_deny_2();
            outbound_acl_deny_and_continue_2();
        }
        default_action = outbound_acl_deny_2();
    }
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] @name("dash_ingress.outbound.outbound_routing_stage.routing") table outbound_outbound_routing_stage_routing {
        key = {
            meta.eni_id          : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.is_overlay_ip_v6: exact @SaiVal[name="destination_is_v6"] @name("meta.is_overlay_ip_v6");
            meta.dst_ip_addr     : lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            route_vnet_0();
            route_vnet_direct_0();
            route_direct_0();
            route_service_tunnel_0();
            drop_1();
        }
        const default_action = drop_1();
    }
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] @name("dash_ingress.outbound.outbound_mapping_stage.ca_to_pa") table outbound_outbound_mapping_stage_ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @SaiVal[type="sai_object_id_t"] @name("meta.dst_vnet_id");
            meta.is_lkup_dst_ip_v6: exact @SaiVal[name="dip_is_v6"] @name("meta.is_lkup_dst_ip_v6");
            meta.lkup_dst_ip_addr : exact @SaiVal[name="dip"] @name("meta.lkup_dst_ip_addr");
        }
        actions = {
            set_tunnel_mapping_0();
            set_private_link_mapping_0();
            @defaultonly drop_2();
        }
        const default_action = drop_2();
    }
    @name("dash_ingress.outbound.outbound_mapping_stage.set_vnet_attrs") action outbound_outbound_mapping_stage_set_vnet_attrs_0(@name("vni") bit<24> vni_2) {
        meta.encap_data.vni = vni_2;
    }
    @SaiTable[name="vnet", api="dash_vnet", isobject="true"] @name("dash_ingress.outbound.outbound_mapping_stage.vnet") table outbound_outbound_mapping_stage_vnet {
        key = {
            meta.vnet_id: exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
        }
        actions = {
            outbound_outbound_mapping_stage_set_vnet_attrs_0();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_0() {
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_1() {
    }
    @name("dash_ingress.inbound.acl.permit") action inbound_acl_permit_2() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_0() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_1() {
    }
    @name("dash_ingress.inbound.acl.permit_and_continue") action inbound_acl_permit_and_continue_2() {
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_2() {
        meta.dropped = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_2() {
        meta.dropped = true;
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", isobject="true"] @name("dash_ingress.inbound.acl.stage1") table inbound_acl_stage1 {
        key = {
            meta.stage1_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_0();
            inbound_acl_permit_and_continue_0();
            inbound_acl_deny_0();
            inbound_acl_deny_and_continue_0();
        }
        default_action = inbound_acl_deny_0();
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", isobject="true"] @name("dash_ingress.inbound.acl.stage2") table inbound_acl_stage2 {
        key = {
            meta.stage2_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage2_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_1();
            inbound_acl_permit_and_continue_1();
            inbound_acl_deny_1();
            inbound_acl_deny_and_continue_1();
        }
        default_action = inbound_acl_deny_1();
    }
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", isobject="true"] @name("dash_ingress.inbound.acl.stage3") table inbound_acl_stage3 {
        key = {
            meta.stage3_dash_acl_group_id: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage3_dash_acl_group_id");
            meta.dst_ip_addr             : ternary @SaiVal[name="dip", type="sai_ip_prefix_list_t"] @name("meta.dst_ip_addr");
            meta.src_ip_addr             : ternary @SaiVal[name="sip", type="sai_ip_prefix_list_t"] @name("meta.src_ip_addr");
            meta.ip_protocol             : ternary @SaiVal[name="protocol", type="sai_u8_list_t"] @name("meta.ip_protocol");
            meta.src_l4_port             : range @SaiVal[name="src_port", type="sai_u16_range_list_t"] @name("meta.src_l4_port");
            meta.dst_l4_port             : range @SaiVal[name="dst_port", type="sai_u16_range_list_t"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_2();
            inbound_acl_permit_and_continue_2();
            inbound_acl_deny_2();
            inbound_acl_deny_and_continue_2();
        }
        default_action = inbound_acl_deny_2();
    }
    @name("dash_ingress.underlay.pkt_act") action underlay_pkt_act_0(@name("packet_action") bit<9> packet_action, @name("next_hop_id") bit<9> next_hop_id) {
        if (packet_action == 9w0) {
            meta.dropped = true;
        } else {
            ;
        }
    }
    @name("dash_ingress.underlay.def_act") action underlay_def_act_0() {
    }
    @SaiTable[name="route", api="route", api_type="underlay"] @name("dash_ingress.underlay.underlay_routing") table underlay_underlay_routing {
        key = {
            meta.dst_ip_addr: lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            underlay_pkt_act_0();
            @defaultonly underlay_def_act_0();
            @defaultonly NoAction_4();
        }
        default_action = NoAction_4();
    }
    @name("dash_ingress.metering_update_stage.check_ip_addr_family") action metering_update_stage_check_ip_addr_family_0(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family_2) {
        if (ip_addr_family_2 == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", order=1, isobject="true"] @name("dash_ingress.metering_update_stage.meter_policy") table metering_update_stage_meter_policy {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id");
        }
        actions = {
            metering_update_stage_check_ip_addr_family_0();
            @defaultonly NoAction_5();
        }
        default_action = NoAction_5();
    }
    @name("dash_ingress.metering_update_stage.set_policy_meter_class") action metering_update_stage_set_policy_meter_class_0(@name("meter_class") bit<16> meter_class_15) {
        meta.policy_meter_class = meter_class_15;
    }
    @SaiTable[name="meter_rule", api="dash_meter", order=2, isobject="true"] @name("dash_ingress.metering_update_stage.meter_rule") table metering_update_stage_meter_rule {
        key = {
            meta.meter_policy_id: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"] @name("meta.meter_policy_id");
            hdr.u0_ipv4.dst_addr: ternary @SaiVal[name="dip", type="sai_ip_address_t"] @name("hdr.u0_ipv4.dst_addr");
        }
        actions = {
            metering_update_stage_set_policy_meter_class_0();
            @defaultonly NoAction_6();
        }
        const default_action = NoAction_6();
    }
    @name("dash_ingress.metering_update_stage.meter_bucket_action") action metering_update_stage_meter_bucket_action_0(@SaiVal[type="sai_uint32_t", skipattr="true"] @name("meter_bucket_index") bit<32> meter_bucket_index_1) {
        meta.meter_bucket_index = meter_bucket_index_1;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", order=0, isobject="true"] @name("dash_ingress.metering_update_stage.meter_bucket") table metering_update_stage_meter_bucket {
        key = {
            meta.eni_id     : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.meter_class: exact @name("meta.meter_class");
        }
        actions = {
            metering_update_stage_meter_bucket_action_0();
            @defaultonly NoAction_7();
        }
        const default_action = NoAction_7();
    }
    @SaiTable[ignored="true"] @name("dash_ingress.metering_update_stage.eni_meter") table metering_update_stage_eni_meter {
        key = {
            meta.eni_id   : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta.direction: exact @name("meta.direction");
            meta.dropped  : exact @name("meta.dropped");
        }
        actions = {
            NoAction_8();
        }
        default_action = NoAction_8();
    }
    apply {
        if (vip_0.apply().hit) {
            meta.encap_data.underlay_sip = hdr.u0_ipv4.dst_addr;
        } else {
            ;
        }
        direction_lookup_stage_direction_lookup.apply();
        appliance_0.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            eni_lookup_stage_tmp = hdr.customer_ethernet.src_addr;
        } else {
            eni_lookup_stage_tmp = hdr.customer_ethernet.dst_addr;
        }
        meta.eni_addr = eni_lookup_stage_tmp;
        if (eni_lookup_stage_eni_ether_address_map.apply().hit) {
            ;
        } else {
            ;
        }
        meta.eni_data.dscp_mode = dash_tunnel_dscp_mode_t.PRESERVE_MODEL;
        meta.eni_data.dscp = (bit<6>)hdr.u0_ipv4.diffserv;
        if (meta.direction == dash_direction_t.OUTBOUND) {
            tunnel_decap_2();
        } else if (meta.direction == dash_direction_t.INBOUND) {
            switch (inbound_routing_0.apply().action_run) {
                tunnel_decap_pa_validate: {
                    pa_validation_0.apply();
                    tunnel_decap_3();
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
        eni_0.apply();
        if (meta.eni_data.admin_state == 1w0) {
            deny_4();
        }
        acl_group_0.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            meta.target_stage = dash_pipeline_stage_t.OUTBOUND_ROUTING;
            if (meta.conntrack_data.allow_out) {
                ;
            } else {
                outbound_acl_hasReturned = false;
                if (meta.stage1_dash_acl_group_id != 16w0) {
                    switch (outbound_acl_stage1.apply().action_run) {
                        outbound_acl_permit_0: {
                            outbound_acl_hasReturned = true;
                        }
                        outbound_acl_deny_0: {
                            outbound_acl_hasReturned = true;
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta.stage2_dash_acl_group_id != 16w0) {
                    switch (outbound_acl_stage2.apply().action_run) {
                        outbound_acl_permit_1: {
                            outbound_acl_hasReturned = true;
                        }
                        outbound_acl_deny_1: {
                            outbound_acl_hasReturned = true;
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta.stage3_dash_acl_group_id != 16w0) {
                    switch (outbound_acl_stage3.apply().action_run) {
                        outbound_acl_permit_2: {
                        }
                        outbound_acl_deny_2: {
                        }
                        default: {
                        }
                    }
                }
            }
            meta.lkup_dst_ip_addr = meta.dst_ip_addr;
            meta.is_lkup_dst_ip_v6 = meta.is_overlay_ip_v6;
            outbound_outbound_routing_stage_hasReturned = false;
            if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_ROUTING) {
                outbound_outbound_routing_stage_hasReturned = true;
            }
            if (outbound_outbound_routing_stage_hasReturned) {
                ;
            } else {
                outbound_outbound_routing_stage_routing.apply();
            }
            outbound_outbound_mapping_stage_hasReturned = false;
            if (meta.target_stage != dash_pipeline_stage_t.OUTBOUND_MAPPING) {
                outbound_outbound_mapping_stage_hasReturned = true;
            }
            if (outbound_outbound_mapping_stage_hasReturned) {
                ;
            } else {
                switch (outbound_outbound_mapping_stage_ca_to_pa.apply().action_run) {
                    set_tunnel_mapping_0: {
                        outbound_outbound_mapping_stage_vnet.apply();
                    }
                    default: {
                    }
                }
            }
        } else if (meta.direction == dash_direction_t.INBOUND) {
            if (meta.conntrack_data.allow_in) {
                ;
            } else {
                inbound_acl_hasReturned = false;
                if (meta.stage1_dash_acl_group_id != 16w0) {
                    switch (inbound_acl_stage1.apply().action_run) {
                        inbound_acl_permit_0: {
                            inbound_acl_hasReturned = true;
                        }
                        inbound_acl_deny_0: {
                            inbound_acl_hasReturned = true;
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta.stage2_dash_acl_group_id != 16w0) {
                    switch (inbound_acl_stage2.apply().action_run) {
                        inbound_acl_permit_1: {
                            inbound_acl_hasReturned = true;
                        }
                        inbound_acl_deny_1: {
                            inbound_acl_hasReturned = true;
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta.stage3_dash_acl_group_id != 16w0) {
                    switch (inbound_acl_stage3.apply().action_run) {
                        inbound_acl_permit_2: {
                        }
                        inbound_acl_deny_2: {
                        }
                        default: {
                        }
                    }
                }
            }
            if (meta.tunnel_pointer == 16w0) {
                push_vxlan_tunnel_u0_1();
            } else if (meta.tunnel_pointer == 16w1) {
                push_vxlan_tunnel_u1_1();
            }
            meta.tunnel_pointer = meta.tunnel_pointer + 16w1;
        }
        routing_action_apply_do_action_nat46_hasReturned = false;
        if (meta.routing_actions & 32w2 == 32w0) {
            routing_action_apply_do_action_nat46_hasReturned = true;
        }
        if (routing_action_apply_do_action_nat46_hasReturned) {
            ;
        } else {
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
        routing_action_apply_do_action_nat64_hasReturned = false;
        if (meta.routing_actions & 32w4 == 32w0) {
            routing_action_apply_do_action_nat64_hasReturned = true;
        }
        if (routing_action_apply_do_action_nat64_hasReturned) {
            ;
        } else {
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
        routing_action_apply_do_action_static_encap_hasReturned = false;
        if (meta.routing_actions & 32w1 == 32w0) {
            routing_action_apply_do_action_static_encap_hasReturned = true;
        }
        if (routing_action_apply_do_action_static_encap_hasReturned) {
            ;
        } else {
            if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
                if (meta.tunnel_pointer == 16w0) {
                    push_vxlan_tunnel_u0_2();
                } else if (meta.tunnel_pointer == 16w1) {
                    push_vxlan_tunnel_u1_2();
                }
            } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
                if (meta.tunnel_pointer == 16w0) {
                    push_vxlan_tunnel_u0_3();
                } else if (meta.tunnel_pointer == 16w1) {
                    push_vxlan_tunnel_u1_3();
                }
            }
            meta.tunnel_pointer = meta.tunnel_pointer + 16w1;
        }
        meta.dst_ip_addr = (bit<128>)hdr.u0_ipv4.dst_addr;
        underlay_underlay_routing.apply();
        if (meta.eni_data.dscp_mode == dash_tunnel_dscp_mode_t.PIPE_MODEL) {
            hdr.u0_ipv4.diffserv = (bit<8>)meta.eni_data.dscp;
        }
        if (meta.meter_policy_en == 1w1) {
            metering_update_stage_meter_policy.apply();
            metering_update_stage_meter_rule.apply();
        }
        if (meta.meter_policy_en == 1w1) {
            meta.meter_class = meta.policy_meter_class;
        } else {
            meta.meter_class = meta.route_meter_class;
        }
        if (meta.meter_class == 16w0 || meta.mapping_meter_class_override == 1w1) {
            meta.meter_class = meta.mapping_meter_class;
        }
        metering_update_stage_meter_bucket.apply();
        metering_update_stage_eni_meter.apply();
        if (meta.dropped) {
            drop_action();
        }
    }
}

control dash_precontrol(in headers_t pre_hdr, inout metadata_t pre_user_meta, in pna_pre_input_metadata_t istd, inout pna_pre_output_metadata_t ostd) {
    apply {
    }
}

PNA_NIC<headers_t, metadata_t, headers_t, metadata_t>(dash_parser(), dash_precontrol(), dash_ingress(), dash_deparser()) main;
