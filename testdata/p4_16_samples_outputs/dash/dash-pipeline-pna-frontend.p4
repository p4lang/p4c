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

typedef bit<32> tag_map_t;
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
    bit<32> cps;
    bit<32> pps;
    bit<32> flows;
    bit<1>  admin_state;
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
    tag_map_t        src_tag_map;
    tag_map_t        dst_tag_map;
}

parser dash_parser(packet_in packet, out headers_t hd, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
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

control dash_deparser(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
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

match_kind {
    list,
    range_list
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("dash_ingress.inner_ip_len") bit<16> inner_ip_len_0;
    @name("dash_ingress.inner_ip_len") bit<16> inner_ip_len_1;
    @name("dash_ingress.inner_ip_len") bit<16> inner_ip_len_5;
    @name("dash_ingress.inner_ip_len") bit<16> inner_ip_len_6;
    @name("dash_ingress.tmp_2") bit<48> tmp_0;
    @name("dash_ingress.outbound.tmp") bit<32> outbound_tmp;
    @name("dash_ingress.outbound.tmp_0") bit<32> outbound_tmp_0;
    @name("dash_ingress.hdr") headers_t hdr_0;
    @name("dash_ingress.st_dst") IPv6Address st_dst;
    @name("dash_ingress.st_dst_mask") IPv6Address st_dst_mask;
    @name("dash_ingress.st_src") IPv6Address st_src;
    @name("dash_ingress.st_src_mask") IPv6Address st_src_mask;
    @name("dash_ingress.inbound.tmp_1") bit<48> inbound_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    @name("dash_ingress.hdr") headers_t hdr_1;
    @name("dash_ingress.hdr") headers_t hdr_2;
    @name("dash_ingress.hdr") headers_t hdr_11;
    @name("dash_ingress.hdr") headers_t hdr_12;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_0;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_0;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_0;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_0;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_0;
    @name("dash_ingress.vni") bit<24> vni_0;
    @name("dash_ingress.hdr") headers_t hdr_13;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_1;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_1;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_1;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_1;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_1;
    @name("dash_ingress.vni") bit<24> vni_1;
    @name("dash_ingress.hdr") headers_t hdr_14;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_6;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_6;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_2;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_2;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_2;
    @name("dash_ingress.vni") bit<24> vni_6;
    @name("dash_ingress.hdr") headers_t hdr_15;
    @name("dash_ingress.underlay_dmac") EthernetAddress underlay_dmac_7;
    @name("dash_ingress.underlay_smac") EthernetAddress underlay_smac_7;
    @name("dash_ingress.underlay_dip") IPv4Address underlay_dip_3;
    @name("dash_ingress.underlay_sip") IPv4Address underlay_sip_8;
    @name("dash_ingress.overlay_dmac") EthernetAddress overlay_dmac_8;
    @name("dash_ingress.vsid") bit<24> vsid_0;
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
    @noWarn("unused") @name(".NoAction") action NoAction_9() {
    }
    @name(".vxlan_decap") action vxlan_decap_1() {
        hdr_1 = hdr;
        hdr_1.ethernet = hdr_1.inner_ethernet;
        hdr_1.inner_ethernet.setInvalid();
        hdr_1.ipv4 = hdr_1.inner_ipv4;
        hdr_1.inner_ipv4.setInvalid();
        hdr_1.ipv6 = hdr_1.inner_ipv6;
        hdr_1.inner_ipv6.setInvalid();
        hdr_1.vxlan.setInvalid();
        hdr_1.udp.setInvalid();
        hdr_1.tcp = hdr_1.inner_tcp;
        hdr_1.inner_tcp.setInvalid();
        hdr_1.udp = hdr_1.inner_udp;
        hdr_1.inner_udp.setInvalid();
        hdr = hdr_1;
    }
    @name(".vxlan_decap") action vxlan_decap_2() {
        hdr_2 = hdr;
        hdr_2.ethernet = hdr_2.inner_ethernet;
        hdr_2.inner_ethernet.setInvalid();
        hdr_2.ipv4 = hdr_2.inner_ipv4;
        hdr_2.inner_ipv4.setInvalid();
        hdr_2.ipv6 = hdr_2.inner_ipv6;
        hdr_2.inner_ipv6.setInvalid();
        hdr_2.vxlan.setInvalid();
        hdr_2.udp.setInvalid();
        hdr_2.tcp = hdr_2.inner_tcp;
        hdr_2.inner_tcp.setInvalid();
        hdr_2.udp = hdr_2.inner_udp;
        hdr_2.inner_udp.setInvalid();
        hdr = hdr_2;
    }
    @name(".vxlan_decap") action vxlan_decap_3() {
        hdr_11 = hdr;
        hdr_11.ethernet = hdr_11.inner_ethernet;
        hdr_11.inner_ethernet.setInvalid();
        hdr_11.ipv4 = hdr_11.inner_ipv4;
        hdr_11.inner_ipv4.setInvalid();
        hdr_11.ipv6 = hdr_11.inner_ipv6;
        hdr_11.inner_ipv6.setInvalid();
        hdr_11.vxlan.setInvalid();
        hdr_11.udp.setInvalid();
        hdr_11.tcp = hdr_11.inner_tcp;
        hdr_11.inner_tcp.setInvalid();
        hdr_11.udp = hdr_11.inner_udp;
        hdr_11.inner_udp.setInvalid();
        hdr = hdr_11;
    }
    @name(".vxlan_encap") action vxlan_encap_1() {
        hdr_12 = hdr;
        underlay_dmac_0 = meta.encap_data.underlay_dmac;
        underlay_smac_0 = meta.encap_data.underlay_smac;
        underlay_dip_0 = meta.encap_data.underlay_dip;
        underlay_sip_0 = meta.encap_data.underlay_sip;
        overlay_dmac_0 = meta.encap_data.overlay_dmac;
        vni_0 = meta.encap_data.vni;
        hdr_12.inner_ethernet = hdr_12.ethernet;
        hdr_12.inner_ethernet.dst_addr = overlay_dmac_0;
        hdr_12.ethernet.setInvalid();
        hdr_12.inner_ipv4 = hdr_12.ipv4;
        hdr_12.ipv4.setInvalid();
        hdr_12.inner_ipv6 = hdr_12.ipv6;
        hdr_12.ipv6.setInvalid();
        hdr_12.inner_tcp = hdr_12.tcp;
        hdr_12.tcp.setInvalid();
        hdr_12.inner_udp = hdr_12.udp;
        hdr_12.udp.setInvalid();
        hdr_12.ethernet.setValid();
        hdr_12.ethernet.dst_addr = underlay_dmac_0;
        hdr_12.ethernet.src_addr = underlay_smac_0;
        hdr_12.ethernet.ether_type = 16w0x800;
        hdr_12.ipv4.setValid();
        hdr_12.ipv4.version = 4w4;
        hdr_12.ipv4.ihl = 4w5;
        hdr_12.ipv4.diffserv = 8w0;
        inner_ip_len_0 = 16w0;
        if (hdr_12.inner_ipv4.isValid()) {
            inner_ip_len_0 = inner_ip_len_0 + hdr_12.inner_ipv4.total_len;
        }
        if (hdr_12.inner_ipv6.isValid()) {
            inner_ip_len_0 = inner_ip_len_0 + 16w40 + hdr_12.inner_ipv6.payload_length;
        }
        hdr_12.ipv4.total_len = 16w50 + inner_ip_len_0;
        hdr_12.ipv4.identification = 16w1;
        hdr_12.ipv4.flags = 3w0;
        hdr_12.ipv4.frag_offset = 13w0;
        hdr_12.ipv4.ttl = 8w64;
        hdr_12.ipv4.protocol = 8w17;
        hdr_12.ipv4.dst_addr = underlay_dip_0;
        hdr_12.ipv4.src_addr = underlay_sip_0;
        hdr_12.ipv4.hdr_checksum = 16w0;
        hdr_12.udp.setValid();
        hdr_12.udp.src_port = 16w0;
        hdr_12.udp.dst_port = 16w4789;
        hdr_12.udp.length = 16w30 + inner_ip_len_0;
        hdr_12.udp.checksum = 16w0;
        hdr_12.vxlan.setValid();
        hdr_12.vxlan.reserved = 24w0;
        hdr_12.vxlan.reserved_2 = 8w0;
        hdr_12.vxlan.flags = 8w0;
        hdr_12.vxlan.vni = vni_0;
        hdr = hdr_12;
    }
    @name(".vxlan_encap") action vxlan_encap_2() {
        hdr_13 = hdr;
        underlay_dmac_1 = meta.encap_data.underlay_dmac;
        underlay_smac_1 = meta.encap_data.underlay_smac;
        underlay_dip_1 = meta.encap_data.underlay_dip;
        underlay_sip_1 = meta.encap_data.underlay_sip;
        overlay_dmac_1 = meta.encap_data.overlay_dmac;
        vni_1 = meta.encap_data.service_tunnel_key;
        hdr_13.inner_ethernet = hdr_13.ethernet;
        hdr_13.inner_ethernet.dst_addr = overlay_dmac_1;
        hdr_13.ethernet.setInvalid();
        hdr_13.inner_ipv4 = hdr_13.ipv4;
        hdr_13.ipv4.setInvalid();
        hdr_13.inner_ipv6 = hdr_13.ipv6;
        hdr_13.ipv6.setInvalid();
        hdr_13.inner_tcp = hdr_13.tcp;
        hdr_13.tcp.setInvalid();
        hdr_13.inner_udp = hdr_13.udp;
        hdr_13.udp.setInvalid();
        hdr_13.ethernet.setValid();
        hdr_13.ethernet.dst_addr = underlay_dmac_1;
        hdr_13.ethernet.src_addr = underlay_smac_1;
        hdr_13.ethernet.ether_type = 16w0x800;
        hdr_13.ipv4.setValid();
        hdr_13.ipv4.version = 4w4;
        hdr_13.ipv4.ihl = 4w5;
        hdr_13.ipv4.diffserv = 8w0;
        inner_ip_len_1 = 16w0;
        if (hdr_13.inner_ipv4.isValid()) {
            inner_ip_len_1 = inner_ip_len_1 + hdr_13.inner_ipv4.total_len;
        }
        if (hdr_13.inner_ipv6.isValid()) {
            inner_ip_len_1 = inner_ip_len_1 + 16w40 + hdr_13.inner_ipv6.payload_length;
        }
        hdr_13.ipv4.total_len = 16w50 + inner_ip_len_1;
        hdr_13.ipv4.identification = 16w1;
        hdr_13.ipv4.flags = 3w0;
        hdr_13.ipv4.frag_offset = 13w0;
        hdr_13.ipv4.ttl = 8w64;
        hdr_13.ipv4.protocol = 8w17;
        hdr_13.ipv4.dst_addr = underlay_dip_1;
        hdr_13.ipv4.src_addr = underlay_sip_1;
        hdr_13.ipv4.hdr_checksum = 16w0;
        hdr_13.udp.setValid();
        hdr_13.udp.src_port = 16w0;
        hdr_13.udp.dst_port = 16w4789;
        hdr_13.udp.length = 16w30 + inner_ip_len_1;
        hdr_13.udp.checksum = 16w0;
        hdr_13.vxlan.setValid();
        hdr_13.vxlan.reserved = 24w0;
        hdr_13.vxlan.reserved_2 = 8w0;
        hdr_13.vxlan.flags = 8w0;
        hdr_13.vxlan.vni = vni_1;
        hdr = hdr_13;
    }
    @name(".vxlan_encap") action vxlan_encap_3() {
        hdr_14 = hdr;
        underlay_dmac_6 = meta.encap_data.underlay_dmac;
        underlay_smac_6 = meta.encap_data.underlay_smac;
        underlay_dip_2 = meta.encap_data.underlay_dip;
        underlay_sip_2 = meta.encap_data.underlay_sip;
        overlay_dmac_2 = inbound_tmp;
        vni_6 = meta.encap_data.vni;
        hdr_14.inner_ethernet = hdr_14.ethernet;
        hdr_14.inner_ethernet.dst_addr = overlay_dmac_2;
        hdr_14.ethernet.setInvalid();
        hdr_14.inner_ipv4 = hdr_14.ipv4;
        hdr_14.ipv4.setInvalid();
        hdr_14.inner_ipv6 = hdr_14.ipv6;
        hdr_14.ipv6.setInvalid();
        hdr_14.inner_tcp = hdr_14.tcp;
        hdr_14.tcp.setInvalid();
        hdr_14.inner_udp = hdr_14.udp;
        hdr_14.udp.setInvalid();
        hdr_14.ethernet.setValid();
        hdr_14.ethernet.dst_addr = underlay_dmac_6;
        hdr_14.ethernet.src_addr = underlay_smac_6;
        hdr_14.ethernet.ether_type = 16w0x800;
        hdr_14.ipv4.setValid();
        hdr_14.ipv4.version = 4w4;
        hdr_14.ipv4.ihl = 4w5;
        hdr_14.ipv4.diffserv = 8w0;
        inner_ip_len_5 = 16w0;
        if (hdr_14.inner_ipv4.isValid()) {
            inner_ip_len_5 = inner_ip_len_5 + hdr_14.inner_ipv4.total_len;
        }
        if (hdr_14.inner_ipv6.isValid()) {
            inner_ip_len_5 = inner_ip_len_5 + 16w40 + hdr_14.inner_ipv6.payload_length;
        }
        hdr_14.ipv4.total_len = 16w50 + inner_ip_len_5;
        hdr_14.ipv4.identification = 16w1;
        hdr_14.ipv4.flags = 3w0;
        hdr_14.ipv4.frag_offset = 13w0;
        hdr_14.ipv4.ttl = 8w64;
        hdr_14.ipv4.protocol = 8w17;
        hdr_14.ipv4.dst_addr = underlay_dip_2;
        hdr_14.ipv4.src_addr = underlay_sip_2;
        hdr_14.ipv4.hdr_checksum = 16w0;
        hdr_14.udp.setValid();
        hdr_14.udp.src_port = 16w0;
        hdr_14.udp.dst_port = 16w4789;
        hdr_14.udp.length = 16w30 + inner_ip_len_5;
        hdr_14.udp.checksum = 16w0;
        hdr_14.vxlan.setValid();
        hdr_14.vxlan.reserved = 24w0;
        hdr_14.vxlan.reserved_2 = 8w0;
        hdr_14.vxlan.flags = 8w0;
        hdr_14.vxlan.vni = vni_6;
        hdr = hdr_14;
    }
    @name(".nvgre_encap") action nvgre_encap_0() {
        hdr_15 = hdr;
        underlay_dmac_7 = meta.encap_data.underlay_dmac;
        underlay_smac_7 = meta.encap_data.underlay_smac;
        underlay_dip_3 = meta.encap_data.underlay_dip;
        underlay_sip_8 = meta.encap_data.underlay_sip;
        overlay_dmac_8 = meta.encap_data.overlay_dmac;
        vsid_0 = meta.encap_data.service_tunnel_key;
        hdr_15.inner_ethernet = hdr_15.ethernet;
        hdr_15.inner_ethernet.dst_addr = overlay_dmac_8;
        hdr_15.ethernet.setInvalid();
        hdr_15.inner_ipv4 = hdr_15.ipv4;
        hdr_15.ipv4.setInvalid();
        hdr_15.inner_ipv6 = hdr_15.ipv6;
        hdr_15.ipv6.setInvalid();
        hdr_15.inner_tcp = hdr_15.tcp;
        hdr_15.tcp.setInvalid();
        hdr_15.inner_udp = hdr_15.udp;
        hdr_15.udp.setInvalid();
        hdr_15.ethernet.setValid();
        hdr_15.ethernet.dst_addr = underlay_dmac_7;
        hdr_15.ethernet.src_addr = underlay_smac_7;
        hdr_15.ethernet.ether_type = 16w0x800;
        hdr_15.ipv4.setValid();
        hdr_15.ipv4.version = 4w4;
        hdr_15.ipv4.ihl = 4w5;
        hdr_15.ipv4.diffserv = 8w0;
        inner_ip_len_6 = 16w0;
        if (hdr_15.inner_ipv4.isValid()) {
            inner_ip_len_6 = inner_ip_len_6 + hdr_15.inner_ipv4.total_len;
        }
        if (hdr_15.inner_ipv6.isValid()) {
            inner_ip_len_6 = inner_ip_len_6 + 16w40 + hdr_15.inner_ipv6.payload_length;
        }
        hdr_15.ipv4.total_len = 16w50 + inner_ip_len_6;
        hdr_15.ipv4.identification = 16w1;
        hdr_15.ipv4.flags = 3w0;
        hdr_15.ipv4.frag_offset = 13w0;
        hdr_15.ipv4.ttl = 8w64;
        hdr_15.ipv4.protocol = 8w0x2f;
        hdr_15.ipv4.dst_addr = underlay_dip_3;
        hdr_15.ipv4.src_addr = underlay_sip_8;
        hdr_15.ipv4.hdr_checksum = 16w0;
        hdr_15.nvgre.setValid();
        hdr_15.nvgre.flags = 4w4;
        hdr_15.nvgre.reserved = 9w0;
        hdr_15.nvgre.version = 3w0;
        hdr_15.nvgre.protocol_type = 16w0x6558;
        hdr_15.nvgre.vsid = vsid_0;
        hdr_15.nvgre.flow_id = 8w0;
        hdr = hdr_15;
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
    @name("dash_ingress.deny") action deny_2() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_3() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_4() {
        meta.dropped = true;
    }
    @name("dash_ingress.deny") action deny_5() {
        meta.dropped = true;
    }
    @name("dash_ingress.accept") action accept_1() {
    }
    @name("dash_ingress.vip|dash_vip") table vip_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr:VIP");
        }
        actions = {
            accept_1();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @name("dash_ingress.set_outbound_direction") action set_outbound_direction() {
        meta.direction = dash_direction_t.OUTBOUND;
    }
    @name("dash_ingress.set_inbound_direction") action set_inbound_direction() {
        meta.direction = dash_direction_t.INBOUND;
    }
    @name("dash_ingress.direction_lookup|dash_direction_lookup") table direction_lookup_0 {
        key = {
            hdr.vxlan.vni: exact @name("hdr.vxlan.vni:VNI");
        }
        actions = {
            set_outbound_direction();
            @defaultonly set_inbound_direction();
        }
        const default_action = set_inbound_direction();
    }
    @name("dash_ingress.set_appliance") action set_appliance(@name("neighbor_mac") EthernetAddress neighbor_mac, @name("mac") EthernetAddress mac) {
        meta.encap_data.underlay_dmac = neighbor_mac;
        meta.encap_data.underlay_smac = mac;
    }
    @name("dash_ingress.appliance") table appliance_0 {
        key = {
            meta.appliance_id: ternary @name("meta.appliance_id:appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @name("vm_underlay_dip") IPv4Address vm_underlay_dip, @Sai[type="sai_uint32_t"] @name("vm_vni") bit<24> vm_vni, @name("vnet_id") bit<16> vnet_id_1, @name("v4_meter_policy_id") bit<16> v4_meter_policy_id, @name("v6_meter_policy_id") bit<16> v6_meter_policy_id, @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta.eni_data.cps = cps_1;
        meta.eni_data.pps = pps_1;
        meta.eni_data.flows = flows_1;
        meta.eni_data.admin_state = admin_state_1;
        meta.encap_data.underlay_dip = vm_underlay_dip;
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
    }
    @name("dash_ingress.eni|dash_eni") table eni_0 {
        key = {
            meta.eni_id: exact @name("meta.eni_id:eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny_0();
        }
        const default_action = deny_0();
    }
    @name("dash_ingress.eni_meter") table eni_meter_0 {
        key = {
            meta.eni_id   : exact @name("meta.eni_id:eni_id");
            meta.direction: exact @name("meta.direction:direction");
            meta.dropped  : exact @name("meta.dropped:dropped");
        }
        actions = {
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.vxlan_decap_pa_validate") action vxlan_decap_pa_validate(@name("src_vnet_id") bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @name("dash_ingress.pa_validation|dash_pa_validation") table pa_validation_0 {
        key = {
            meta.vnet_id     : exact @name("meta.vnet_id:vnet_id");
            hdr.ipv4.src_addr: exact @name("hdr.ipv4.src_addr:sip");
        }
        actions = {
            permit();
            @defaultonly deny_2();
        }
        const default_action = deny_2();
    }
    @name("dash_ingress.inbound_routing|dash_inbound_routing") table inbound_routing_0 {
        key = {
            meta.eni_id      : exact @name("meta.eni_id:eni_id");
            hdr.vxlan.vni    : exact @name("hdr.vxlan.vni:VNI");
            hdr.ipv4.src_addr: ternary @name("hdr.ipv4.src_addr:sip");
        }
        actions = {
            vxlan_decap_1();
            vxlan_decap_pa_validate();
            @defaultonly deny_3();
        }
        const default_action = deny_3();
    }
    @name("dash_ingress.check_ip_addr_family") action check_ip_addr_family(@Sai[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @name("dash_ingress.meter_policy|dash_meter") @Sai[isobject="true"] table meter_policy_0 {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id:meter_policy_id");
        }
        actions = {
            check_ip_addr_family();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("dash_ingress.set_policy_meter_class") action set_policy_meter_class(@name("meter_class") bit<16> meter_class_0) {
        meta.policy_meter_class = meter_class_0;
    }
    @name("dash_ingress.meter_rule|dash_meter") @Sai[isobject="true"] table meter_rule_0 {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id:meter_policy_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"];
            hdr.ipv4.dst_addr   : ternary @name("hdr.ipv4.dst_addr:dip");
        }
        actions = {
            set_policy_meter_class();
            @defaultonly NoAction_4();
        }
        const default_action = NoAction_4();
    }
    @name("dash_ingress.meter_bucket_action") action meter_bucket_action(@Sai[type="sai_uint64_t", isreadonly="true"] @name("outbound_bytes_counter") bit<64> outbound_bytes_counter, @Sai[type="sai_uint64_t", isreadonly="true"] @name("inbound_bytes_counter") bit<64> inbound_bytes_counter, @Sai[type="sai_uint32_t", skipattr="true"] @name("meter_bucket_index") bit<32> meter_bucket_index_1) {
        meta.meter_bucket_index = meter_bucket_index_1;
    }
    @name("dash_ingress.meter_bucket|dash_meter") @Sai[isobject="true"] table meter_bucket_0 {
        key = {
            meta.eni_id     : exact @name("meta.eni_id:eni_id");
            meta.meter_class: exact @name("meta.meter_class:meter_class");
        }
        actions = {
            meter_bucket_action();
            @defaultonly NoAction_5();
        }
        const default_action = NoAction_5();
    }
    @name("dash_ingress.set_eni") action set_eni(@name("eni_id") bit<16> eni_id_1) {
        meta.eni_id = eni_id_1;
    }
    @name("dash_ingress.eni_ether_address_map|dash_eni") table eni_ether_address_map_0 {
        key = {
            meta.eni_addr: exact @name("meta.eni_addr:address");
        }
        actions = {
            set_eni();
            @defaultonly deny_4();
        }
        const default_action = deny_4();
    }
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@Sai[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family_2) {
        if (ip_addr_family_2 == 32w0) {
            if (meta.is_overlay_ip_v6 == 1w1) {
                meta.dropped = true;
            }
        } else if (meta.is_overlay_ip_v6 == 1w0) {
            meta.dropped = true;
        }
    }
    @name("dash_ingress.dash_acl_group|dash_acl") table acl_group_0 {
        key = {
            meta.stage1_dash_acl_group_id: exact @name("meta.stage1_dash_acl_group_id:dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_6();
        }
        default_action = NoAction_6();
    }
    @name("dash_ingress.set_src_tag") action set_src_tag(@name("tag_map") tag_map_t tag_map) {
        meta.src_tag_map = tag_map;
    }
    @name("dash_ingress.src_tag|dash_tag") table src_tag_0 {
        key = {
            meta.src_ip_addr: lpm @name("meta.src_ip_addr:sip");
        }
        actions = {
            set_src_tag();
            @defaultonly NoAction_7();
        }
        default_action = NoAction_7();
    }
    @name("dash_ingress.set_dst_tag") action set_dst_tag(@name("tag_map") tag_map_t tag_map_2) {
        meta.dst_tag_map = tag_map_2;
    }
    @name("dash_ingress.dst_tag|dash_tag") table dst_tag_0 {
        key = {
            meta.dst_ip_addr: lpm @name("meta.dst_ip_addr:dip");
        }
        actions = {
            set_dst_tag();
            @defaultonly NoAction_8();
        }
        default_action = NoAction_8();
    }
    @name("dash_ingress.outbound.route_vnet") action outbound_route_vnet_0(@name("dst_vnet_id") bit<16> dst_vnet_id_2, @name("meter_policy_en") bit<1> meter_policy_en_0, @name("meter_class") bit<16> meter_class_7) {
        meta.dst_vnet_id = dst_vnet_id_2;
        meta.meter_policy_en = meter_policy_en_0;
        meta.route_meter_class = meter_class_7;
    }
    @name("dash_ingress.outbound.route_vnet_direct") action outbound_route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("is_overlay_ip_v4_or_v6") bit<1> is_overlay_ip_v4_or_v6, @name("overlay_ip") IPv4ORv6Address overlay_ip, @name("meter_policy_en") bit<1> meter_policy_en_5, @name("meter_class") bit<16> meter_class_8) {
        meta.dst_vnet_id = dst_vnet_id_3;
        meta.lkup_dst_ip_addr = overlay_ip;
        meta.is_lkup_dst_ip_v6 = is_overlay_ip_v4_or_v6;
        meta.meter_policy_en = meter_policy_en_5;
        meta.route_meter_class = meter_class_8;
    }
    @name("dash_ingress.outbound.route_direct") action outbound_route_direct_0(@name("meter_policy_en") bit<1> meter_policy_en_6, @name("meter_class") bit<16> meter_class_9) {
        meta.meter_policy_en = meter_policy_en_6;
        meta.route_meter_class = meter_class_9;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_0() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_1() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_2() {
        meta.dropped = true;
    }
    @name("dash_ingress.outbound.route_service_tunnel") action outbound_route_service_tunnel_0(@name("is_overlay_dip_v4_or_v6") bit<1> is_overlay_dip_v4_or_v6, @name("overlay_dip") IPv4ORv6Address overlay_dip, @name("is_overlay_dip_mask_v4_or_v6") bit<1> is_overlay_dip_mask_v4_or_v6, @name("overlay_dip_mask") IPv4ORv6Address overlay_dip_mask, @name("is_overlay_sip_v4_or_v6") bit<1> is_overlay_sip_v4_or_v6, @name("overlay_sip") IPv4ORv6Address overlay_sip, @name("is_overlay_sip_mask_v4_or_v6") bit<1> is_overlay_sip_mask_v4_or_v6, @name("overlay_sip_mask") IPv4ORv6Address overlay_sip_mask, @name("is_underlay_dip_v4_or_v6") bit<1> is_underlay_dip_v4_or_v6, @name("underlay_dip") IPv4ORv6Address underlay_dip_8, @name("is_underlay_sip_v4_or_v6") bit<1> is_underlay_sip_v4_or_v6, @name("underlay_sip") IPv4ORv6Address underlay_sip_7, @name("dash_encapsulation") dash_encapsulation_t dash_encapsulation_1, @name("tunnel_key") bit<24> tunnel_key, @name("meter_policy_en") bit<1> meter_policy_en_7, @name("meter_class") bit<16> meter_class_10) {
        meta.encap_data.original_overlay_dip = hdr.ipv4.src_addr;
        meta.encap_data.original_overlay_sip = hdr.ipv4.dst_addr;
        hdr_0 = hdr;
        st_dst = overlay_dip;
        st_dst_mask = overlay_dip_mask;
        st_src = overlay_sip;
        st_src_mask = overlay_sip_mask;
        hdr_0.ipv6.setValid();
        hdr_0.ipv6.version = 4w6;
        hdr_0.ipv6.traffic_class = 8w0;
        hdr_0.ipv6.flow_label = 20w0;
        hdr_0.ipv6.payload_length = hdr_0.ipv4.total_len + 16w65516;
        hdr_0.ipv6.next_header = hdr_0.ipv4.protocol;
        hdr_0.ipv6.hop_limit = hdr_0.ipv4.ttl;
        hdr_0.ipv6.dst_addr = (IPv6Address)hdr_0.ipv4.dst_addr & ~st_dst_mask | st_dst & st_dst_mask;
        hdr_0.ipv6.src_addr = (IPv6Address)hdr_0.ipv4.src_addr & ~st_src_mask | st_src & st_src_mask;
        hdr_0.ipv4.setInvalid();
        hdr_0.ethernet.ether_type = 16w0x86dd;
        hdr = hdr_0;
        if (underlay_dip_8 == 128w0) {
            outbound_tmp = meta.encap_data.original_overlay_dip;
        } else {
            outbound_tmp = (IPv4Address)underlay_dip_8;
        }
        meta.encap_data.underlay_dip = outbound_tmp;
        if (underlay_sip_7 == 128w0) {
            outbound_tmp_0 = meta.encap_data.original_overlay_sip;
        } else {
            outbound_tmp_0 = (IPv4Address)underlay_sip_7;
        }
        meta.encap_data.underlay_sip = outbound_tmp_0;
        meta.encap_data.overlay_dmac = hdr.ethernet.dst_addr;
        meta.encap_data.dash_encapsulation = dash_encapsulation_1;
        meta.encap_data.service_tunnel_key = tunnel_key;
        meta.meter_policy_en = meter_policy_en_7;
        meta.route_meter_class = meter_class_10;
    }
    @name("dash_ingress.outbound.outbound_routing|dash_outbound_routing") table outbound_outbound_routing_dash_outbound_routing {
        key = {
            meta.eni_id          : exact @name("meta.eni_id:eni_id");
            meta.is_overlay_ip_v6: exact @name("meta.is_overlay_ip_v6:is_destination_v4_or_v6");
            meta.dst_ip_addr     : lpm @name("meta.dst_ip_addr:destination");
        }
        actions = {
            outbound_route_vnet_0();
            outbound_route_vnet_direct_0();
            outbound_route_direct_0();
            outbound_route_service_tunnel_0();
            outbound_drop_0();
        }
        const default_action = outbound_drop_0();
    }
    @name("dash_ingress.outbound.set_tunnel_mapping") action outbound_set_tunnel_mapping_0(@name("underlay_dip") IPv4Address underlay_dip_9, @name("overlay_dmac") EthernetAddress overlay_dmac_7, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni, @name("meter_class") bit<16> meter_class_11, @name("meter_class_override") bit<1> meter_class_override) {
        if (use_dst_vnet_vni == 1w1) {
            meta.vnet_id = meta.dst_vnet_id;
        }
        meta.encap_data.overlay_dmac = overlay_dmac_7;
        meta.encap_data.underlay_dip = underlay_dip_9;
        meta.mapping_meter_class = meter_class_11;
        meta.mapping_meter_class_override = meter_class_override;
    }
    @name("dash_ingress.outbound.outbound_ca_to_pa|dash_outbound_ca_to_pa") table outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @name("meta.dst_vnet_id:dst_vnet_id");
            meta.is_lkup_dst_ip_v6: exact @name("meta.is_lkup_dst_ip_v6:is_dip_v4_or_v6");
            meta.lkup_dst_ip_addr : exact @name("meta.lkup_dst_ip_addr:dip");
        }
        actions = {
            outbound_set_tunnel_mapping_0();
            @defaultonly outbound_drop_1();
        }
        const default_action = outbound_drop_1();
    }
    @name("dash_ingress.outbound.set_vnet_attrs") action outbound_set_vnet_attrs_0(@name("vni") bit<24> vni_5) {
        meta.encap_data.vni = vni_5;
    }
    @name("dash_ingress.outbound.vnet|dash_vnet") table outbound_vnet_dash_vnet {
        key = {
            meta.vnet_id: exact @name("meta.vnet_id:vnet_id");
        }
        actions = {
            outbound_set_vnet_attrs_0();
            @defaultonly NoAction_9();
        }
        default_action = NoAction_9();
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
    @name("dash_ingress.outbound.acl.stage1:dash_acl_rule|dash_acl") table outbound_acl_stage1_dash_acl_rule_dash_acl {
        key = {
            meta.stage1_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            outbound_acl_permit_0();
            outbound_acl_permit_and_continue_0();
            outbound_acl_deny_0();
            outbound_acl_deny_and_continue_0();
        }
        default_action = outbound_acl_deny_0();
    }
    @name("dash_ingress.outbound.acl.stage2:dash_acl_rule|dash_acl") table outbound_acl_stage2_dash_acl_rule_dash_acl {
        key = {
            meta.stage2_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            outbound_acl_permit_1();
            outbound_acl_permit_and_continue_1();
            outbound_acl_deny_1();
            outbound_acl_deny_and_continue_1();
        }
        default_action = outbound_acl_deny_1();
    }
    @name("dash_ingress.outbound.acl.stage3:dash_acl_rule|dash_acl") table outbound_acl_stage3_dash_acl_rule_dash_acl {
        key = {
            meta.stage3_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            outbound_acl_permit_2();
            outbound_acl_permit_and_continue_2();
            outbound_acl_deny_2();
            outbound_acl_deny_and_continue_2();
        }
        default_action = outbound_acl_deny_2();
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
    @name("dash_ingress.inbound.acl.stage1:dash_acl_rule|dash_acl") table inbound_acl_stage1_dash_acl_rule_dash_acl {
        key = {
            meta.stage1_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            inbound_acl_permit_0();
            inbound_acl_permit_and_continue_0();
            inbound_acl_deny_0();
            inbound_acl_deny_and_continue_0();
        }
        default_action = inbound_acl_deny_0();
    }
    @name("dash_ingress.inbound.acl.stage2:dash_acl_rule|dash_acl") table inbound_acl_stage2_dash_acl_rule_dash_acl {
        key = {
            meta.stage2_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            inbound_acl_permit_1();
            inbound_acl_permit_and_continue_1();
            inbound_acl_deny_1();
            inbound_acl_deny_and_continue_1();
        }
        default_action = inbound_acl_deny_1();
    }
    @name("dash_ingress.inbound.acl.stage3:dash_acl_rule|dash_acl") table inbound_acl_stage3_dash_acl_rule_dash_acl {
        key = {
            meta.stage3_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            inbound_acl_permit_2();
            inbound_acl_permit_and_continue_2();
            inbound_acl_deny_2();
            inbound_acl_deny_and_continue_2();
        }
        default_action = inbound_acl_deny_2();
    }
    apply {
        if (vip_0.apply().hit) {
            meta.encap_data.underlay_sip = hdr.ipv4.dst_addr;
        }
        direction_lookup_0.apply();
        appliance_0.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            vxlan_decap_2();
        } else if (meta.direction == dash_direction_t.INBOUND) {
            switch (inbound_routing_0.apply().action_run) {
                vxlan_decap_pa_validate: {
                    pa_validation_0.apply();
                    vxlan_decap_3();
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
        if (meta.direction == dash_direction_t.OUTBOUND) {
            tmp_0 = hdr.ethernet.src_addr;
        } else {
            tmp_0 = hdr.ethernet.dst_addr;
        }
        meta.eni_addr = tmp_0;
        eni_ether_address_map_0.apply();
        eni_0.apply();
        if (meta.eni_data.admin_state == 1w0) {
            deny_5();
        }
        acl_group_0.apply();
        src_tag_0.apply();
        dst_tag_0.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            if (meta.conntrack_data.allow_out) {
                ;
            } else {
                outbound_acl_hasReturned = false;
                if (meta.stage1_dash_acl_group_id != 16w0) {
                    switch (outbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
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
                    switch (outbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
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
                    switch (outbound_acl_stage3_dash_acl_rule_dash_acl.apply().action_run) {
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
            switch (outbound_outbound_routing_dash_outbound_routing.apply().action_run) {
                outbound_route_vnet_direct_0: 
                outbound_route_vnet_0: {
                    outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa.apply();
                    outbound_vnet_dash_vnet.apply();
                    vxlan_encap_1();
                }
                outbound_route_service_tunnel_0: {
                    if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.VXLAN) {
                        vxlan_encap_2();
                    } else if (meta.encap_data.dash_encapsulation == dash_encapsulation_t.NVGRE) {
                        nvgre_encap_0();
                    } else {
                        outbound_drop_2();
                    }
                }
                default: {
                }
            }
        } else if (meta.direction == dash_direction_t.INBOUND) {
            if (meta.conntrack_data.allow_in) {
                ;
            } else {
                inbound_acl_hasReturned = false;
                if (meta.stage1_dash_acl_group_id != 16w0) {
                    switch (inbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
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
                    switch (inbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
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
                    switch (inbound_acl_stage3_dash_acl_rule_dash_acl.apply().action_run) {
                        inbound_acl_permit_2: {
                        }
                        inbound_acl_deny_2: {
                        }
                        default: {
                        }
                    }
                }
            }
            inbound_tmp = hdr.ethernet.dst_addr;
            vxlan_encap_3();
        }
        if (meta.meter_policy_en == 1w1) {
            meter_policy_0.apply();
            meter_rule_0.apply();
        }
        if (meta.meter_policy_en == 1w1) {
            meta.meter_class = meta.policy_meter_class;
        } else {
            meta.meter_class = meta.route_meter_class;
        }
        if (meta.meter_class == 16w0 || meta.mapping_meter_class_override == 1w1) {
            meta.meter_class = meta.mapping_meter_class;
        }
        meter_bucket_0.apply();
        eni_meter_0.apply();
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
