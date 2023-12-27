error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    InvalidIPv4Header
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
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
    bit<4>   version;
    bit<8>   traffic_class;
    bit<20>  flow_label;
    bit<16>  payload_length;
    bit<8>   next_header;
    bit<8>   hop_limit;
    bit<128> src_addr;
    bit<128> dst_addr;
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

struct encap_data_t {
    bit<24> vni;
    bit<24> dest_vnet_vni;
    bit<32> underlay_sip;
    bit<32> underlay_dip;
    bit<48> underlay_smac;
    bit<48> underlay_dmac;
    bit<48> overlay_dmac;
    bit<16> dash_encapsulation;
    bit<24> service_tunnel_key;
    bit<32> original_overlay_sip;
    bit<32> original_overlay_dip;
}

struct conntrack_data_t {
    bool allow_in;
    bool allow_out;
}

struct eni_data_t {
    bit<32>  cps;
    bit<32>  pps;
    bit<32>  flows;
    bit<1>   admin_state;
    bit<128> pl_sip;
    bit<128> pl_sip_mask;
    bit<32>  pl_underlay_sip;
}

struct metadata_t {
    bool     _dropped0;
    bit<16>  _direction1;
    bit<24>  _encap_data_vni2;
    bit<24>  _encap_data_dest_vnet_vni3;
    bit<32>  _encap_data_underlay_sip4;
    bit<32>  _encap_data_underlay_dip5;
    bit<48>  _encap_data_underlay_smac6;
    bit<48>  _encap_data_underlay_dmac7;
    bit<48>  _encap_data_overlay_dmac8;
    bit<16>  _encap_data_dash_encapsulation9;
    bit<24>  _encap_data_service_tunnel_key10;
    bit<32>  _encap_data_original_overlay_sip11;
    bit<32>  _encap_data_original_overlay_dip12;
    bit<48>  _eni_addr13;
    bit<16>  _vnet_id14;
    bit<16>  _dst_vnet_id15;
    bit<16>  _eni_id16;
    bit<32>  _eni_data_cps17;
    bit<32>  _eni_data_pps18;
    bit<32>  _eni_data_flows19;
    bit<1>   _eni_data_admin_state20;
    bit<128> _eni_data_pl_sip21;
    bit<128> _eni_data_pl_sip_mask22;
    bit<32>  _eni_data_pl_underlay_sip23;
    bit<16>  _inbound_vm_id24;
    bit<8>   _appliance_id25;
    bit<1>   _is_overlay_ip_v626;
    bit<1>   _is_lkup_dst_ip_v627;
    bit<8>   _ip_protocol28;
    bit<128> _dst_ip_addr29;
    bit<128> _src_ip_addr30;
    bit<128> _lkup_dst_ip_addr31;
    bool     _conntrack_data_allow_in32;
    bool     _conntrack_data_allow_out33;
    bit<16>  _src_l4_port34;
    bit<16>  _dst_l4_port35;
    bit<16>  _stage1_dash_acl_group_id36;
    bit<16>  _stage2_dash_acl_group_id37;
    bit<16>  _stage3_dash_acl_group_id38;
    bit<16>  _stage4_dash_acl_group_id39;
    bit<16>  _stage5_dash_acl_group_id40;
    bit<1>   _meter_policy_en41;
    bit<1>   _mapping_meter_class_override42;
    bit<16>  _meter_policy_id43;
    bit<16>  _policy_meter_class44;
    bit<16>  _route_meter_class45;
    bit<16>  _mapping_meter_class46;
    bit<16>  _meter_class47;
    bit<32>  _meter_bucket_index48;
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

match_kind {
    list,
    range_list
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @name("dash_ingress.tmp_3") bit<48> tmp_0;
    @name("dash_ingress.outbound.tmp") bit<32> outbound_tmp;
    @name("dash_ingress.outbound.tmp_0") bit<32> outbound_tmp_0;
    ethernet_t hdr_0_ethernet;
    ipv4_t hdr_0_ipv4;
    ipv6_t hdr_0_ipv6;
    ethernet_t hdr_1_ethernet;
    ipv4_t hdr_1_ipv4;
    ipv6_t hdr_1_ipv6;
    @name("dash_ingress.inbound.tmp_2") bit<48> inbound_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    udp_t hdr_2_udp;
    vxlan_t hdr_2_vxlan;
    ethernet_t hdr_2_inner_ethernet;
    ipv4_t hdr_2_inner_ipv4;
    ipv6_t hdr_2_inner_ipv6;
    udp_t hdr_2_inner_udp;
    tcp_t hdr_2_inner_tcp;
    udp_t hdr_13_udp;
    vxlan_t hdr_13_vxlan;
    ethernet_t hdr_13_inner_ethernet;
    ipv4_t hdr_13_inner_ipv4;
    ipv6_t hdr_13_inner_ipv6;
    udp_t hdr_13_inner_udp;
    tcp_t hdr_13_inner_tcp;
    udp_t hdr_14_udp;
    vxlan_t hdr_14_vxlan;
    ethernet_t hdr_14_inner_ethernet;
    ipv4_t hdr_14_inner_ipv4;
    ipv6_t hdr_14_inner_ipv6;
    udp_t hdr_14_inner_udp;
    tcp_t hdr_14_inner_tcp;
    ethernet_t hdr_15_ethernet;
    ipv4_t hdr_15_ipv4;
    ipv6_t hdr_15_ipv6;
    udp_t hdr_15_udp;
    tcp_t hdr_15_tcp;
    vxlan_t hdr_15_vxlan;
    ethernet_t hdr_15_inner_ethernet;
    ipv4_t hdr_15_inner_ipv4;
    ipv6_t hdr_15_inner_ipv6;
    udp_t hdr_15_inner_udp;
    tcp_t hdr_15_inner_tcp;
    ethernet_t hdr_16_ethernet;
    ipv4_t hdr_16_ipv4;
    ipv6_t hdr_16_ipv6;
    udp_t hdr_16_udp;
    tcp_t hdr_16_tcp;
    vxlan_t hdr_16_vxlan;
    ethernet_t hdr_16_inner_ethernet;
    ipv4_t hdr_16_inner_ipv4;
    ipv6_t hdr_16_inner_ipv6;
    udp_t hdr_16_inner_udp;
    tcp_t hdr_16_inner_tcp;
    ethernet_t hdr_17_ethernet;
    ipv4_t hdr_17_ipv4;
    ipv6_t hdr_17_ipv6;
    udp_t hdr_17_udp;
    tcp_t hdr_17_tcp;
    vxlan_t hdr_17_vxlan;
    ethernet_t hdr_17_inner_ethernet;
    ipv4_t hdr_17_inner_ipv4;
    ipv6_t hdr_17_inner_ipv6;
    udp_t hdr_17_inner_udp;
    tcp_t hdr_17_inner_tcp;
    ethernet_t hdr_18_ethernet;
    ipv4_t hdr_18_ipv4;
    ipv6_t hdr_18_ipv6;
    udp_t hdr_18_udp;
    tcp_t hdr_18_tcp;
    nvgre_t hdr_18_nvgre;
    ethernet_t hdr_18_inner_ethernet;
    ipv4_t hdr_18_inner_ipv4;
    ipv6_t hdr_18_inner_ipv6;
    udp_t hdr_18_inner_udp;
    tcp_t hdr_18_inner_tcp;
    ethernet_t hdr_19_ethernet;
    ipv4_t hdr_19_ipv4;
    ipv6_t hdr_19_ipv6;
    udp_t hdr_19_udp;
    tcp_t hdr_19_tcp;
    nvgre_t hdr_19_nvgre;
    ethernet_t hdr_19_inner_ethernet;
    ipv4_t hdr_19_inner_ipv4;
    ipv6_t hdr_19_inner_ipv6;
    udp_t hdr_19_inner_udp;
    tcp_t hdr_19_inner_tcp;
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
    @name(".vxlan_decap") action vxlan_decap_1() {
        hdr_2_udp = hdr.udp;
        hdr_2_vxlan = hdr.vxlan;
        hdr_2_inner_ethernet = hdr.inner_ethernet;
        hdr_2_inner_ipv4 = hdr.inner_ipv4;
        hdr_2_inner_ipv6 = hdr.inner_ipv6;
        hdr_2_inner_udp = hdr.inner_udp;
        hdr_2_inner_tcp = hdr.inner_tcp;
        hdr_2_inner_ethernet.setInvalid();
        hdr_2_inner_ipv4.setInvalid();
        hdr_2_inner_ipv6.setInvalid();
        hdr_2_vxlan.setInvalid();
        hdr_2_udp.setInvalid();
        hdr_2_inner_tcp.setInvalid();
        hdr_2_udp = hdr.inner_udp;
        hdr_2_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_2_vxlan;
        hdr.inner_ethernet = hdr_2_inner_ethernet;
        hdr.inner_ipv4 = hdr_2_inner_ipv4;
        hdr.inner_ipv6 = hdr_2_inner_ipv6;
        hdr.inner_udp = hdr_2_inner_udp;
        hdr.inner_tcp = hdr_2_inner_tcp;
    }
    @name(".vxlan_decap") action vxlan_decap_2() {
        hdr_13_udp = hdr.udp;
        hdr_13_vxlan = hdr.vxlan;
        hdr_13_inner_ethernet = hdr.inner_ethernet;
        hdr_13_inner_ipv4 = hdr.inner_ipv4;
        hdr_13_inner_ipv6 = hdr.inner_ipv6;
        hdr_13_inner_udp = hdr.inner_udp;
        hdr_13_inner_tcp = hdr.inner_tcp;
        hdr_13_inner_ethernet.setInvalid();
        hdr_13_inner_ipv4.setInvalid();
        hdr_13_inner_ipv6.setInvalid();
        hdr_13_vxlan.setInvalid();
        hdr_13_udp.setInvalid();
        hdr_13_inner_tcp.setInvalid();
        hdr_13_udp = hdr.inner_udp;
        hdr_13_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_13_vxlan;
        hdr.inner_ethernet = hdr_13_inner_ethernet;
        hdr.inner_ipv4 = hdr_13_inner_ipv4;
        hdr.inner_ipv6 = hdr_13_inner_ipv6;
        hdr.inner_udp = hdr_13_inner_udp;
        hdr.inner_tcp = hdr_13_inner_tcp;
    }
    @name(".vxlan_decap") action vxlan_decap_3() {
        hdr_14_udp = hdr.udp;
        hdr_14_vxlan = hdr.vxlan;
        hdr_14_inner_ethernet = hdr.inner_ethernet;
        hdr_14_inner_ipv4 = hdr.inner_ipv4;
        hdr_14_inner_ipv6 = hdr.inner_ipv6;
        hdr_14_inner_udp = hdr.inner_udp;
        hdr_14_inner_tcp = hdr.inner_tcp;
        hdr_14_inner_ethernet.setInvalid();
        hdr_14_inner_ipv4.setInvalid();
        hdr_14_inner_ipv6.setInvalid();
        hdr_14_vxlan.setInvalid();
        hdr_14_udp.setInvalid();
        hdr_14_inner_tcp.setInvalid();
        hdr_14_udp = hdr.inner_udp;
        hdr_14_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_14_vxlan;
        hdr.inner_ethernet = hdr_14_inner_ethernet;
        hdr.inner_ipv4 = hdr_14_inner_ipv4;
        hdr.inner_ipv6 = hdr_14_inner_ipv6;
        hdr.inner_udp = hdr_14_inner_udp;
        hdr.inner_tcp = hdr_14_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_1() {
        hdr_15_ethernet = hdr.ethernet;
        hdr_15_ipv4 = hdr.ipv4;
        hdr_15_ipv6 = hdr.ipv6;
        hdr_15_udp = hdr.udp;
        hdr_15_tcp = hdr.tcp;
        hdr_15_vxlan = hdr.vxlan;
        hdr_15_inner_ethernet = hdr.inner_ethernet;
        hdr_15_inner_ipv4 = hdr.inner_ipv4;
        hdr_15_inner_ipv6 = hdr.inner_ipv6;
        hdr_15_inner_udp = hdr.inner_udp;
        hdr_15_inner_tcp = hdr.inner_tcp;
        hdr_15_inner_ethernet = hdr.ethernet;
        hdr_15_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_15_ethernet.setInvalid();
        hdr_15_inner_ipv4 = hdr.ipv4;
        hdr_15_ipv4.setInvalid();
        hdr_15_inner_ipv6 = hdr.ipv6;
        hdr_15_ipv6.setInvalid();
        hdr_15_inner_tcp = hdr.tcp;
        hdr_15_tcp.setInvalid();
        hdr_15_inner_udp = hdr.udp;
        hdr_15_udp.setInvalid();
        hdr_15_ethernet.setValid();
        hdr_15_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_15_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_15_ethernet.ether_type = 16w0x800;
        hdr_15_ipv4.setValid();
        hdr_15_ipv4.version = 4w4;
        hdr_15_ipv4.ihl = 4w5;
        hdr_15_ipv4.diffserv = 8w0;
        hdr_15_ipv4.total_len = hdr_15_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_15_inner_ipv4.isValid() + hdr_15_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w50;
        hdr_15_ipv4.identification = 16w1;
        hdr_15_ipv4.flags = 3w0;
        hdr_15_ipv4.frag_offset = 13w0;
        hdr_15_ipv4.ttl = 8w64;
        hdr_15_ipv4.protocol = 8w17;
        hdr_15_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_15_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_15_ipv4.hdr_checksum = 16w0;
        hdr_15_udp.setValid();
        hdr_15_udp.src_port = 16w0;
        hdr_15_udp.dst_port = 16w4789;
        hdr_15_udp.length = hdr_15_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_15_inner_ipv4.isValid() + hdr_15_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w30;
        hdr_15_udp.checksum = 16w0;
        hdr_15_vxlan.setValid();
        hdr_15_vxlan.reserved = 24w0;
        hdr_15_vxlan.reserved_2 = 8w0;
        hdr_15_vxlan.flags = 8w0;
        hdr_15_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_15_ethernet;
        hdr.ipv4 = hdr_15_ipv4;
        hdr.ipv6 = hdr_15_ipv6;
        hdr.udp = hdr_15_udp;
        hdr.tcp = hdr_15_tcp;
        hdr.vxlan = hdr_15_vxlan;
        hdr.inner_ethernet = hdr_15_inner_ethernet;
        hdr.inner_ipv4 = hdr_15_inner_ipv4;
        hdr.inner_ipv6 = hdr_15_inner_ipv6;
        hdr.inner_udp = hdr_15_inner_udp;
        hdr.inner_tcp = hdr_15_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_2() {
        hdr_16_ethernet = hdr.ethernet;
        hdr_16_ipv4 = hdr.ipv4;
        hdr_16_ipv6 = hdr.ipv6;
        hdr_16_udp = hdr.udp;
        hdr_16_tcp = hdr.tcp;
        hdr_16_vxlan = hdr.vxlan;
        hdr_16_inner_ethernet = hdr.inner_ethernet;
        hdr_16_inner_ipv4 = hdr.inner_ipv4;
        hdr_16_inner_ipv6 = hdr.inner_ipv6;
        hdr_16_inner_udp = hdr.inner_udp;
        hdr_16_inner_tcp = hdr.inner_tcp;
        hdr_16_inner_ethernet = hdr.ethernet;
        hdr_16_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_16_ethernet.setInvalid();
        hdr_16_inner_ipv4 = hdr.ipv4;
        hdr_16_ipv4.setInvalid();
        hdr_16_inner_ipv6 = hdr.ipv6;
        hdr_16_ipv6.setInvalid();
        hdr_16_inner_tcp = hdr.tcp;
        hdr_16_tcp.setInvalid();
        hdr_16_inner_udp = hdr.udp;
        hdr_16_udp.setInvalid();
        hdr_16_ethernet.setValid();
        hdr_16_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_16_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_16_ethernet.ether_type = 16w0x800;
        hdr_16_ipv4.setValid();
        hdr_16_ipv4.version = 4w4;
        hdr_16_ipv4.ihl = 4w5;
        hdr_16_ipv4.diffserv = 8w0;
        hdr_16_ipv4.total_len = hdr_16_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_16_inner_ipv4.isValid() + hdr_16_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_16_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_16_inner_ipv6.isValid() + 16w50;
        hdr_16_ipv4.identification = 16w1;
        hdr_16_ipv4.flags = 3w0;
        hdr_16_ipv4.frag_offset = 13w0;
        hdr_16_ipv4.ttl = 8w64;
        hdr_16_ipv4.protocol = 8w17;
        hdr_16_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_16_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_16_ipv4.hdr_checksum = 16w0;
        hdr_16_udp.setValid();
        hdr_16_udp.src_port = 16w0;
        hdr_16_udp.dst_port = 16w4789;
        hdr_16_udp.length = hdr_16_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_16_inner_ipv4.isValid() + hdr_16_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_16_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_16_inner_ipv6.isValid() + 16w30;
        hdr_16_udp.checksum = 16w0;
        hdr_16_vxlan.setValid();
        hdr_16_vxlan.reserved = 24w0;
        hdr_16_vxlan.reserved_2 = 8w0;
        hdr_16_vxlan.flags = 8w0;
        hdr_16_vxlan.vni = meta._encap_data_service_tunnel_key10;
        hdr.ethernet = hdr_16_ethernet;
        hdr.ipv4 = hdr_16_ipv4;
        hdr.ipv6 = hdr_16_ipv6;
        hdr.udp = hdr_16_udp;
        hdr.tcp = hdr_16_tcp;
        hdr.vxlan = hdr_16_vxlan;
        hdr.inner_ethernet = hdr_16_inner_ethernet;
        hdr.inner_ipv4 = hdr_16_inner_ipv4;
        hdr.inner_ipv6 = hdr_16_inner_ipv6;
        hdr.inner_udp = hdr_16_inner_udp;
        hdr.inner_tcp = hdr_16_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_3() {
        hdr_17_ethernet = hdr.ethernet;
        hdr_17_ipv4 = hdr.ipv4;
        hdr_17_ipv6 = hdr.ipv6;
        hdr_17_udp = hdr.udp;
        hdr_17_tcp = hdr.tcp;
        hdr_17_vxlan = hdr.vxlan;
        hdr_17_inner_ethernet = hdr.inner_ethernet;
        hdr_17_inner_ipv4 = hdr.inner_ipv4;
        hdr_17_inner_ipv6 = hdr.inner_ipv6;
        hdr_17_inner_udp = hdr.inner_udp;
        hdr_17_inner_tcp = hdr.inner_tcp;
        hdr_17_inner_ethernet = hdr.ethernet;
        hdr_17_inner_ethernet.dst_addr = inbound_tmp;
        hdr_17_ethernet.setInvalid();
        hdr_17_inner_ipv4 = hdr.ipv4;
        hdr_17_ipv4.setInvalid();
        hdr_17_inner_ipv6 = hdr.ipv6;
        hdr_17_ipv6.setInvalid();
        hdr_17_inner_tcp = hdr.tcp;
        hdr_17_tcp.setInvalid();
        hdr_17_inner_udp = hdr.udp;
        hdr_17_udp.setInvalid();
        hdr_17_ethernet.setValid();
        hdr_17_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_17_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_17_ethernet.ether_type = 16w0x800;
        hdr_17_ipv4.setValid();
        hdr_17_ipv4.version = 4w4;
        hdr_17_ipv4.ihl = 4w5;
        hdr_17_ipv4.diffserv = 8w0;
        hdr_17_ipv4.total_len = hdr_17_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_17_inner_ipv4.isValid() + hdr_17_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_17_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_17_inner_ipv6.isValid() + 16w50;
        hdr_17_ipv4.identification = 16w1;
        hdr_17_ipv4.flags = 3w0;
        hdr_17_ipv4.frag_offset = 13w0;
        hdr_17_ipv4.ttl = 8w64;
        hdr_17_ipv4.protocol = 8w17;
        hdr_17_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_17_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_17_ipv4.hdr_checksum = 16w0;
        hdr_17_udp.setValid();
        hdr_17_udp.src_port = 16w0;
        hdr_17_udp.dst_port = 16w4789;
        hdr_17_udp.length = hdr_17_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_17_inner_ipv4.isValid() + hdr_17_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_17_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_17_inner_ipv6.isValid() + 16w30;
        hdr_17_udp.checksum = 16w0;
        hdr_17_vxlan.setValid();
        hdr_17_vxlan.reserved = 24w0;
        hdr_17_vxlan.reserved_2 = 8w0;
        hdr_17_vxlan.flags = 8w0;
        hdr_17_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_17_ethernet;
        hdr.ipv4 = hdr_17_ipv4;
        hdr.ipv6 = hdr_17_ipv6;
        hdr.udp = hdr_17_udp;
        hdr.tcp = hdr_17_tcp;
        hdr.vxlan = hdr_17_vxlan;
        hdr.inner_ethernet = hdr_17_inner_ethernet;
        hdr.inner_ipv4 = hdr_17_inner_ipv4;
        hdr.inner_ipv6 = hdr_17_inner_ipv6;
        hdr.inner_udp = hdr_17_inner_udp;
        hdr.inner_tcp = hdr_17_inner_tcp;
    }
    @name(".nvgre_encap") action nvgre_encap_1() {
        hdr_18_ethernet = hdr.ethernet;
        hdr_18_ipv4 = hdr.ipv4;
        hdr_18_ipv6 = hdr.ipv6;
        hdr_18_udp = hdr.udp;
        hdr_18_tcp = hdr.tcp;
        hdr_18_nvgre = hdr.nvgre;
        hdr_18_inner_ethernet = hdr.inner_ethernet;
        hdr_18_inner_ipv4 = hdr.inner_ipv4;
        hdr_18_inner_ipv6 = hdr.inner_ipv6;
        hdr_18_inner_udp = hdr.inner_udp;
        hdr_18_inner_tcp = hdr.inner_tcp;
        hdr_18_inner_ethernet = hdr.ethernet;
        hdr_18_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_18_ethernet.setInvalid();
        hdr_18_inner_ipv4 = hdr.ipv4;
        hdr_18_ipv4.setInvalid();
        hdr_18_inner_ipv6 = hdr.ipv6;
        hdr_18_ipv6.setInvalid();
        hdr_18_inner_tcp = hdr.tcp;
        hdr_18_tcp.setInvalid();
        hdr_18_inner_udp = hdr.udp;
        hdr_18_udp.setInvalid();
        hdr_18_ethernet.setValid();
        hdr_18_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_18_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_18_ethernet.ether_type = 16w0x800;
        hdr_18_ipv4.setValid();
        hdr_18_ipv4.version = 4w4;
        hdr_18_ipv4.ihl = 4w5;
        hdr_18_ipv4.diffserv = 8w0;
        hdr_18_ipv4.total_len = hdr_18_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_18_inner_ipv4.isValid() + hdr_18_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_18_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_18_inner_ipv6.isValid() + 16w42;
        hdr_18_ipv4.identification = 16w1;
        hdr_18_ipv4.flags = 3w0;
        hdr_18_ipv4.frag_offset = 13w0;
        hdr_18_ipv4.ttl = 8w64;
        hdr_18_ipv4.protocol = 8w0x2f;
        hdr_18_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_18_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_18_ipv4.hdr_checksum = 16w0;
        hdr_18_nvgre.setValid();
        hdr_18_nvgre.flags = 4w4;
        hdr_18_nvgre.reserved = 9w0;
        hdr_18_nvgre.version = 3w0;
        hdr_18_nvgre.protocol_type = 16w0x6558;
        hdr_18_nvgre.vsid = meta._encap_data_vni2;
        hdr_18_nvgre.flow_id = 8w0;
        hdr.ethernet = hdr_18_ethernet;
        hdr.ipv4 = hdr_18_ipv4;
        hdr.ipv6 = hdr_18_ipv6;
        hdr.udp = hdr_18_udp;
        hdr.tcp = hdr_18_tcp;
        hdr.nvgre = hdr_18_nvgre;
        hdr.inner_ethernet = hdr_18_inner_ethernet;
        hdr.inner_ipv4 = hdr_18_inner_ipv4;
        hdr.inner_ipv6 = hdr_18_inner_ipv6;
        hdr.inner_udp = hdr_18_inner_udp;
        hdr.inner_tcp = hdr_18_inner_tcp;
    }
    @name(".nvgre_encap") action nvgre_encap_2() {
        hdr_19_ethernet = hdr.ethernet;
        hdr_19_ipv4 = hdr.ipv4;
        hdr_19_ipv6 = hdr.ipv6;
        hdr_19_udp = hdr.udp;
        hdr_19_tcp = hdr.tcp;
        hdr_19_nvgre = hdr.nvgre;
        hdr_19_inner_ethernet = hdr.inner_ethernet;
        hdr_19_inner_ipv4 = hdr.inner_ipv4;
        hdr_19_inner_ipv6 = hdr.inner_ipv6;
        hdr_19_inner_udp = hdr.inner_udp;
        hdr_19_inner_tcp = hdr.inner_tcp;
        hdr_19_inner_ethernet = hdr.ethernet;
        hdr_19_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_19_ethernet.setInvalid();
        hdr_19_inner_ipv4 = hdr.ipv4;
        hdr_19_ipv4.setInvalid();
        hdr_19_inner_ipv6 = hdr.ipv6;
        hdr_19_ipv6.setInvalid();
        hdr_19_inner_tcp = hdr.tcp;
        hdr_19_tcp.setInvalid();
        hdr_19_inner_udp = hdr.udp;
        hdr_19_udp.setInvalid();
        hdr_19_ethernet.setValid();
        hdr_19_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_19_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_19_ethernet.ether_type = 16w0x800;
        hdr_19_ipv4.setValid();
        hdr_19_ipv4.version = 4w4;
        hdr_19_ipv4.ihl = 4w5;
        hdr_19_ipv4.diffserv = 8w0;
        hdr_19_ipv4.total_len = hdr_19_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_19_inner_ipv4.isValid() + hdr_19_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_19_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_19_inner_ipv6.isValid() + 16w42;
        hdr_19_ipv4.identification = 16w1;
        hdr_19_ipv4.flags = 3w0;
        hdr_19_ipv4.frag_offset = 13w0;
        hdr_19_ipv4.ttl = 8w64;
        hdr_19_ipv4.protocol = 8w0x2f;
        hdr_19_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_19_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_19_ipv4.hdr_checksum = 16w0;
        hdr_19_nvgre.setValid();
        hdr_19_nvgre.flags = 4w4;
        hdr_19_nvgre.reserved = 9w0;
        hdr_19_nvgre.version = 3w0;
        hdr_19_nvgre.protocol_type = 16w0x6558;
        hdr_19_nvgre.vsid = meta._encap_data_service_tunnel_key10;
        hdr_19_nvgre.flow_id = 8w0;
        hdr.ethernet = hdr_19_ethernet;
        hdr.ipv4 = hdr_19_ipv4;
        hdr.ipv6 = hdr_19_ipv6;
        hdr.udp = hdr_19_udp;
        hdr.tcp = hdr_19_tcp;
        hdr.nvgre = hdr_19_nvgre;
        hdr.inner_ethernet = hdr_19_inner_ethernet;
        hdr.inner_ipv4 = hdr_19_inner_ipv4;
        hdr.inner_ipv6 = hdr_19_inner_ipv6;
        hdr.inner_udp = hdr_19_inner_udp;
        hdr.inner_tcp = hdr_19_inner_tcp;
    }
    @name("dash_ingress.drop_action") action drop_action() {
        mark_to_drop(standard_metadata);
    }
    @name("dash_ingress.deny") action deny() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.deny") action deny_0() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.deny") action deny_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.deny") action deny_3() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.deny") action deny_4() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.deny") action deny_5() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.accept") action accept_1() {
    }
    @SaiTable[name="vip", api="dash_vip"] @name("dash_ingress.vip") table vip_0 {
        key = {
            hdr.ipv4.dst_addr: exact @SaiVal[name="VIP", type="sai_ip_address_t"] @name("hdr.ipv4.dst_addr");
        }
        actions = {
            accept_1();
            @defaultonly deny();
        }
        const default_action = deny();
    }
    @name("dash_ingress.set_outbound_direction") action set_outbound_direction() {
        meta._direction1 = 16w1;
    }
    @name("dash_ingress.set_inbound_direction") action set_inbound_direction() {
        meta._direction1 = 16w2;
    }
    @SaiTable[name="direction_lookup", api="dash_direction_lookup"] @name("dash_ingress.direction_lookup") table direction_lookup_0 {
        key = {
            hdr.vxlan.vni: exact @SaiVal[name="VNI"] @name("hdr.vxlan.vni");
        }
        actions = {
            set_outbound_direction();
            @defaultonly set_inbound_direction();
        }
        const default_action = set_inbound_direction();
    }
    @name("dash_ingress.set_appliance") action set_appliance(@name("neighbor_mac") bit<48> neighbor_mac, @name("mac") bit<48> mac) {
        meta._encap_data_underlay_dmac7 = neighbor_mac;
        meta._encap_data_underlay_smac6 = mac;
    }
    @SaiTable[ignored="true"] @name("dash_ingress.appliance") table appliance_0 {
        key = {
            meta._appliance_id25: ternary @name("meta.appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @SaiVal[type="sai_ip_address_t"] @name("vm_underlay_dip") bit<32> vm_underlay_dip, @SaiVal[type="sai_uint32_t"] @name("vm_vni") bit<24> vm_vni, @SaiVal[type="sai_object_id_t"] @name("vnet_id") bit<16> vnet_id_1, @name("pl_sip") bit<128> pl_sip_1, @name("pl_sip_mask") bit<128> pl_sip_mask_1, @SaiVal[type="sai_ip_address_t"] @name("pl_underlay_sip") bit<32> pl_underlay_sip_1, @SaiVal[type="sai_object_id_t"] @name("v4_meter_policy_id") bit<16> v4_meter_policy_id, @SaiVal[type="sai_object_id_t"] @name("v6_meter_policy_id") bit<16> v6_meter_policy_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @SaiVal[type="sai_object_id_t"] @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta._eni_data_cps17 = cps_1;
        meta._eni_data_pps18 = pps_1;
        meta._eni_data_flows19 = flows_1;
        meta._eni_data_admin_state20 = admin_state_1;
        meta._eni_data_pl_sip21 = pl_sip_1;
        meta._eni_data_pl_sip_mask22 = pl_sip_mask_1;
        meta._eni_data_pl_underlay_sip23 = pl_underlay_sip_1;
        meta._encap_data_underlay_dip5 = vm_underlay_dip;
        meta._encap_data_vni2 = vm_vni;
        meta._vnet_id14 = vnet_id_1;
        if (meta._is_overlay_ip_v626 == 1w1) {
            if (meta._direction1 == 16w1) {
                meta._stage1_dash_acl_group_id36 = outbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id37 = outbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id38 = outbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id39 = outbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id40 = outbound_v6_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id36 = inbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id37 = inbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id38 = inbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id39 = inbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id40 = inbound_v6_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id43 = v6_meter_policy_id;
        } else {
            if (meta._direction1 == 16w1) {
                meta._stage1_dash_acl_group_id36 = outbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id37 = outbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id38 = outbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id39 = outbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id40 = outbound_v4_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id36 = inbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id37 = inbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id38 = inbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id39 = inbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id40 = inbound_v4_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id43 = v4_meter_policy_id;
        }
    }
    @SaiTable[name="eni", api="dash_eni", api_order=1, isobject="true"] @name("dash_ingress.eni") table eni_0 {
        key = {
            meta._eni_id16: exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny_0();
        }
        const default_action = deny_0();
    }
    @name("dash_ingress.eni_counter") direct_counter(CounterType.packets_and_bytes) eni_counter_0;
    @SaiTable[ignored="true"] @name("dash_ingress.eni_meter") table eni_meter_0 {
        key = {
            meta._eni_id16  : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._direction1: exact @name("meta.direction");
            meta._dropped0  : exact @name("meta.dropped");
        }
        actions = {
            NoAction_2();
        }
        counters = eni_counter_0;
        default_action = NoAction_2();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.vxlan_decap_pa_validate") action vxlan_decap_pa_validate(@SaiVal[type="sai_object_id_t"] @name("src_vnet_id") bit<16> src_vnet_id) {
        meta._vnet_id14 = src_vnet_id;
    }
    @SaiTable[name="pa_validation", api="dash_pa_validation"] @name("dash_ingress.pa_validation") table pa_validation_0 {
        key = {
            meta._vnet_id14  : exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
            hdr.ipv4.src_addr: exact @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.ipv4.src_addr");
        }
        actions = {
            permit();
            @defaultonly deny_2();
        }
        const default_action = deny_2();
    }
    @SaiTable[name="inbound_routing", api="dash_inbound_routing"] @name("dash_ingress.inbound_routing") table inbound_routing_0 {
        key = {
            meta._eni_id16   : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            hdr.vxlan.vni    : exact @SaiVal[name="VNI"] @name("hdr.vxlan.vni");
            hdr.ipv4.src_addr: ternary @SaiVal[name="sip", type="sai_ip_address_t"] @name("hdr.ipv4.src_addr");
        }
        actions = {
            vxlan_decap_1();
            vxlan_decap_pa_validate();
            @defaultonly deny_3();
        }
        const default_action = deny_3();
    }
    @name("dash_ingress.check_ip_addr_family") action check_ip_addr_family(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family) {
        if (ip_addr_family == 32w0) {
            if (meta._is_overlay_ip_v626 == 1w1) {
                meta._dropped0 = true;
            }
        } else if (meta._is_overlay_ip_v626 == 1w0) {
            meta._dropped0 = true;
        }
    }
    @SaiTable[name="meter_policy", api="dash_meter", api_order=1, isobject="true"] @name("dash_ingress.meter_policy") table meter_policy_0 {
        key = {
            meta._meter_policy_id43: exact @name("meta.meter_policy_id");
        }
        actions = {
            check_ip_addr_family();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("dash_ingress.set_policy_meter_class") action set_policy_meter_class(@name("meter_class") bit<16> meter_class_0) {
        meta._policy_meter_class44 = meter_class_0;
    }
    @SaiTable[name="meter_rule", api="dash_meter", api_order=2, isobject="true"] @name("dash_ingress.meter_rule") table meter_rule_0 {
        key = {
            meta._meter_policy_id43: exact @SaiVal[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"] @name("meta.meter_policy_id");
            hdr.ipv4.dst_addr      : ternary @SaiVal[name="dip", type="sai_ip_address_t"] @name("hdr.ipv4.dst_addr");
        }
        actions = {
            set_policy_meter_class();
            @defaultonly NoAction_4();
        }
        const default_action = NoAction_4();
    }
    @name("dash_ingress.meter_bucket_inbound") counter(32w262144, CounterType.bytes) meter_bucket_inbound_0;
    @name("dash_ingress.meter_bucket_outbound") counter(32w262144, CounterType.bytes) meter_bucket_outbound_0;
    @name("dash_ingress.meter_bucket_action") action meter_bucket_action(@SaiVal[type="sai_uint64_t", isreadonly="true"] @name("outbound_bytes_counter") bit<64> outbound_bytes_counter, @SaiVal[type="sai_uint64_t", isreadonly="true"] @name("inbound_bytes_counter") bit<64> inbound_bytes_counter, @SaiVal[type="sai_uint32_t", skipattr="true"] @name("meter_bucket_index") bit<32> meter_bucket_index_1) {
        meta._meter_bucket_index48 = meter_bucket_index_1;
    }
    @SaiTable[name="meter_bucket", api="dash_meter", api_order=0, isobject="true"] @name("dash_ingress.meter_bucket") table meter_bucket_0 {
        key = {
            meta._eni_id16     : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._meter_class47: exact @name("meta.meter_class");
        }
        actions = {
            meter_bucket_action();
            @defaultonly NoAction_5();
        }
        const default_action = NoAction_5();
    }
    @name("dash_ingress.set_eni") action set_eni(@SaiVal[type="sai_object_id_t"] @name("eni_id") bit<16> eni_id_1) {
        meta._eni_id16 = eni_id_1;
    }
    @SaiTable[name="eni_ether_address_map", api="dash_eni", api_order=0] @name("dash_ingress.eni_ether_address_map") table eni_ether_address_map_0 {
        key = {
            tmp_0: exact @SaiVal[name="address", type="sai_mac_t"] @name("meta.eni_addr");
        }
        actions = {
            set_eni();
            @defaultonly deny_4();
        }
        const default_action = deny_4();
    }
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@SaiVal[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family_2) {
        if (ip_addr_family_2 == 32w0) {
            if (meta._is_overlay_ip_v626 == 1w1) {
                meta._dropped0 = true;
            }
        } else if (meta._is_overlay_ip_v626 == 1w0) {
            meta._dropped0 = true;
        }
    }
    @SaiTable[name="dash_acl_group", api="dash_acl", api_order=0, isobject="true"] @name("dash_ingress.acl_group") table acl_group_0 {
        key = {
            meta._stage1_dash_acl_group_id36: exact @SaiVal[name="dash_acl_group_id"] @name("meta.stage1_dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_6();
        }
        default_action = NoAction_6();
    }
    @name("dash_ingress.outbound.route_vnet") action outbound_route_vnet_0(@SaiVal[type="sai_object_id_t"] @name("dst_vnet_id") bit<16> dst_vnet_id_2, @name("meter_policy_en") bit<1> meter_policy_en_0, @name("meter_class") bit<16> meter_class_5) {
        meta._dst_vnet_id15 = dst_vnet_id_2;
        meta._meter_policy_en41 = meter_policy_en_0;
        meta._route_meter_class45 = meter_class_5;
    }
    @name("dash_ingress.outbound.route_vnet_direct") action outbound_route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("overlay_ip_is_v6") bit<1> overlay_ip_is_v6, @SaiVal[type="sai_ip_address_t"] @name("overlay_ip") bit<128> overlay_ip, @name("meter_policy_en") bit<1> meter_policy_en_5, @name("meter_class") bit<16> meter_class_9) {
        meta._dst_vnet_id15 = dst_vnet_id_3;
        meta._lkup_dst_ip_addr31 = overlay_ip;
        meta._is_lkup_dst_ip_v627 = overlay_ip_is_v6;
        meta._meter_policy_en41 = meter_policy_en_5;
        meta._route_meter_class45 = meter_class_9;
    }
    @name("dash_ingress.outbound.route_direct") action outbound_route_direct_0(@name("meter_policy_en") bit<1> meter_policy_en_6, @name("meter_class") bit<16> meter_class_10) {
        meta._meter_policy_en41 = meter_policy_en_6;
        meta._route_meter_class45 = meter_class_10;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_0() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_3() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.route_service_tunnel") action outbound_route_service_tunnel_0(@name("overlay_dip_is_v6") bit<1> overlay_dip_is_v6, @name("overlay_dip") bit<128> overlay_dip, @name("overlay_dip_mask_is_v6") bit<1> overlay_dip_mask_is_v6, @name("overlay_dip_mask") bit<128> overlay_dip_mask, @name("overlay_sip_is_v6") bit<1> overlay_sip_is_v6, @name("overlay_sip") bit<128> overlay_sip, @name("overlay_sip_mask_is_v6") bit<1> overlay_sip_mask_is_v6, @name("overlay_sip_mask") bit<128> overlay_sip_mask, @name("underlay_dip_is_v6") bit<1> underlay_dip_is_v6, @name("underlay_dip") bit<128> underlay_dip_10, @name("underlay_sip_is_v6") bit<1> underlay_sip_is_v6, @name("underlay_sip") bit<128> underlay_sip_8, @SaiVal[type="sai_dash_encapsulation_t", default_value="SAI_DASH_ENCAPSULATION_VXLAN"] @name("dash_encapsulation") bit<16> dash_encapsulation_1, @name("tunnel_key") bit<24> tunnel_key, @name("meter_policy_en") bit<1> meter_policy_en_7, @name("meter_class") bit<16> meter_class_11) {
        meta._encap_data_original_overlay_dip12 = hdr.ipv4.src_addr;
        meta._encap_data_original_overlay_sip11 = hdr.ipv4.dst_addr;
        hdr_0_ethernet = hdr.ethernet;
        hdr_0_ipv4 = hdr.ipv4;
        hdr_0_ipv6 = hdr.ipv6;
        hdr_0_ipv6.setValid();
        hdr_0_ipv6.version = 4w6;
        hdr_0_ipv6.traffic_class = 8w0;
        hdr_0_ipv6.flow_label = 20w0;
        hdr_0_ipv6.payload_length = hdr_0_ipv4.total_len + 16w65516;
        hdr_0_ipv6.next_header = hdr_0_ipv4.protocol;
        hdr_0_ipv6.hop_limit = hdr_0_ipv4.ttl;
        hdr_0_ipv6.dst_addr = (bit<128>)hdr_0_ipv4.dst_addr & ~overlay_dip_mask | overlay_dip & overlay_dip_mask;
        hdr_0_ipv6.src_addr = (bit<128>)hdr_0_ipv4.src_addr & ~overlay_sip_mask | overlay_sip & overlay_sip_mask;
        hdr_0_ipv4.setInvalid();
        hdr_0_ethernet.ether_type = 16w0x86dd;
        hdr.ethernet = hdr_0_ethernet;
        hdr.ipv4 = hdr_0_ipv4;
        hdr.ipv6 = hdr_0_ipv6;
        if (underlay_dip_10 == 128w0) {
            outbound_tmp = meta._encap_data_original_overlay_dip12;
        } else {
            outbound_tmp = (bit<32>)underlay_dip_10;
        }
        meta._encap_data_underlay_dip5 = outbound_tmp;
        if (underlay_sip_8 == 128w0) {
            outbound_tmp_0 = meta._encap_data_original_overlay_sip11;
        } else {
            outbound_tmp_0 = (bit<32>)underlay_sip_8;
        }
        meta._encap_data_underlay_sip4 = outbound_tmp_0;
        meta._encap_data_overlay_dmac8 = hdr.ethernet.dst_addr;
        meta._encap_data_dash_encapsulation9 = dash_encapsulation_1;
        meta._encap_data_service_tunnel_key10 = tunnel_key;
        meta._meter_policy_en41 = meter_policy_en_7;
        meta._route_meter_class45 = meter_class_11;
    }
    @name("dash_ingress.outbound.routing_counter") direct_counter(CounterType.packets_and_bytes) outbound_routing_counter;
    @SaiTable[name="outbound_routing", api="dash_outbound_routing"] @name("dash_ingress.outbound.routing") table outbound_routing {
        key = {
            meta._eni_id16          : exact @SaiVal[type="sai_object_id_t"] @name("meta.eni_id");
            meta._is_overlay_ip_v626: exact @SaiVal[name="destination_is_v6"] @name("meta.is_overlay_ip_v6");
            meta._dst_ip_addr29     : lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            outbound_route_vnet_0();
            outbound_route_vnet_direct_0();
            outbound_route_direct_0();
            outbound_route_service_tunnel_0();
            outbound_drop_0();
        }
        const default_action = outbound_drop_0();
        counters = outbound_routing_counter;
    }
    @name("dash_ingress.outbound.set_tunnel_mapping") action outbound_set_tunnel_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") bit<32> underlay_dip_11, @name("overlay_dmac") bit<48> overlay_dmac_8, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni, @name("meter_class") bit<16> meter_class_12, @name("meter_class_override") bit<1> meter_class_override) {
        if (use_dst_vnet_vni == 1w1) {
            meta._vnet_id14 = meta._dst_vnet_id15;
        }
        meta._encap_data_overlay_dmac8 = overlay_dmac_8;
        meta._encap_data_underlay_dip5 = underlay_dip_11;
        meta._mapping_meter_class46 = meter_class_12;
        meta._mapping_meter_class_override42 = meter_class_override;
        meta._encap_data_dash_encapsulation9 = 16w1;
    }
    @name("dash_ingress.outbound.set_private_link_mapping") action outbound_set_private_link_mapping_0(@SaiVal[type="sai_ip_address_t"] @name("underlay_dip") bit<32> underlay_dip_12, @name("overlay_sip") bit<128> overlay_sip_2, @name("overlay_dip") bit<128> overlay_dip_2, @SaiVal[type="sai_dash_encapsulation_t"] @name("dash_encapsulation") bit<16> dash_encapsulation_3, @name("tunnel_key") bit<24> tunnel_key_2, @name("meter_class") bit<16> meter_class_13, @name("meter_class_override") bit<1> meter_class_override_0) {
        meta._encap_data_overlay_dmac8 = hdr.ethernet.dst_addr;
        meta._encap_data_vni2 = tunnel_key_2;
        hdr_1_ethernet = hdr.ethernet;
        hdr_1_ipv4 = hdr.ipv4;
        hdr_1_ipv6 = hdr.ipv6;
        hdr_1_ipv6.setValid();
        hdr_1_ipv6.version = 4w6;
        hdr_1_ipv6.traffic_class = 8w0;
        hdr_1_ipv6.flow_label = 20w0;
        hdr_1_ipv6.payload_length = hdr_1_ipv4.total_len + 16w65516;
        hdr_1_ipv6.next_header = hdr_1_ipv4.protocol;
        hdr_1_ipv6.hop_limit = hdr_1_ipv4.ttl;
        hdr_1_ipv6.dst_addr = (bit<128>)hdr_1_ipv4.dst_addr & 128w0xffffffff000000000000000000000000 | overlay_dip_2 & 128w0xffffffffffffffffffffffff;
        hdr_1_ipv6.src_addr = (bit<128>)hdr_1_ipv4.src_addr & 128w0xffffffff000000000000000000000000 | (overlay_sip_2 & ~meta._eni_data_pl_sip_mask22 | meta._eni_data_pl_sip21 | (bit<128>)hdr.ipv4.dst_addr) & 128w0xffffffffffffffffffffffff;
        hdr_1_ipv4.setInvalid();
        hdr_1_ethernet.ether_type = 16w0x86dd;
        hdr.ethernet = hdr_1_ethernet;
        hdr.ipv4 = hdr_1_ipv4;
        hdr.ipv6 = hdr_1_ipv6;
        meta._encap_data_underlay_dip5 = underlay_dip_12;
        meta._mapping_meter_class46 = meter_class_13;
        meta._mapping_meter_class_override42 = meter_class_override_0;
        meta._encap_data_dash_encapsulation9 = dash_encapsulation_3;
    }
    @name("dash_ingress.outbound.ca_to_pa_counter") direct_counter(CounterType.packets_and_bytes) outbound_ca_to_pa_counter;
    @SaiTable[name="outbound_ca_to_pa", api="dash_outbound_ca_to_pa"] @name("dash_ingress.outbound.ca_to_pa") table outbound_ca_to_pa {
        key = {
            meta._dst_vnet_id15      : exact @SaiVal[type="sai_object_id_t"] @name("meta.dst_vnet_id");
            meta._is_lkup_dst_ip_v627: exact @SaiVal[name="dip_is_v6"] @name("meta.is_lkup_dst_ip_v6");
            meta._lkup_dst_ip_addr31 : exact @SaiVal[name="dip"] @name("meta.lkup_dst_ip_addr");
        }
        actions = {
            outbound_set_tunnel_mapping_0();
            outbound_set_private_link_mapping_0();
            @defaultonly outbound_drop_1();
        }
        const default_action = outbound_drop_1();
        counters = outbound_ca_to_pa_counter;
    }
    @name("dash_ingress.outbound.set_vnet_attrs") action outbound_set_vnet_attrs_0(@name("vni") bit<24> vni_5) {
        meta._encap_data_vni2 = vni_5;
    }
    @SaiTable[name="vnet", api="dash_vnet", isobject="true"] @name("dash_ingress.outbound.vnet") table outbound_vnet {
        key = {
            meta._vnet_id14: exact @SaiVal[type="sai_object_id_t"] @name("meta.vnet_id");
        }
        actions = {
            outbound_set_vnet_attrs_0();
            @defaultonly NoAction_7();
        }
        default_action = NoAction_7();
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
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.deny") action outbound_acl_deny_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_0() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.deny_and_continue") action outbound_acl_deny_and_continue_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.acl.stage1_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage1_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage1") table outbound_acl_stage1 {
        key = {
            meta._stage1_dash_acl_group_id36: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage1_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_0();
            outbound_acl_permit_and_continue_0();
            outbound_acl_deny_0();
            outbound_acl_deny_and_continue_0();
        }
        default_action = outbound_acl_deny_0();
        counters = outbound_acl_stage1_counter;
    }
    @name("dash_ingress.outbound.acl.stage2_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage2_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage2") table outbound_acl_stage2 {
        key = {
            meta._stage2_dash_acl_group_id37: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage2_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_1();
            outbound_acl_permit_and_continue_1();
            outbound_acl_deny_1();
            outbound_acl_deny_and_continue_1();
        }
        default_action = outbound_acl_deny_1();
        counters = outbound_acl_stage2_counter;
    }
    @name("dash_ingress.outbound.acl.stage3_counter") direct_counter(CounterType.packets_and_bytes) outbound_acl_stage3_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.outbound.acl.stage3") table outbound_acl_stage3 {
        key = {
            meta._stage3_dash_acl_group_id38: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage3_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            outbound_acl_permit_2();
            outbound_acl_permit_and_continue_2();
            outbound_acl_deny_2();
            outbound_acl_deny_and_continue_2();
        }
        default_action = outbound_acl_deny_2();
        counters = outbound_acl_stage3_counter;
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
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.deny") action inbound_acl_deny_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_0() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.deny_and_continue") action inbound_acl_deny_and_continue_2() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.inbound.acl.stage1_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage1_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage1", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage1") table inbound_acl_stage1 {
        key = {
            meta._stage1_dash_acl_group_id36: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage1_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_0();
            inbound_acl_permit_and_continue_0();
            inbound_acl_deny_0();
            inbound_acl_deny_and_continue_0();
        }
        default_action = inbound_acl_deny_0();
        counters = inbound_acl_stage1_counter;
    }
    @name("dash_ingress.inbound.acl.stage2_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage2_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage2", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage2") table inbound_acl_stage2 {
        key = {
            meta._stage2_dash_acl_group_id37: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage2_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_1();
            inbound_acl_permit_and_continue_1();
            inbound_acl_deny_1();
            inbound_acl_deny_and_continue_1();
        }
        default_action = inbound_acl_deny_1();
        counters = inbound_acl_stage2_counter;
    }
    @name("dash_ingress.inbound.acl.stage3_counter") direct_counter(CounterType.packets_and_bytes) inbound_acl_stage3_counter;
    @SaiTable[name="dash_acl_rule", stage="acl.stage3", api="dash_acl", api_order=1, isobject="true"] @name("dash_ingress.inbound.acl.stage3") table inbound_acl_stage3 {
        key = {
            meta._stage3_dash_acl_group_id38: exact @SaiVal[name="dash_acl_group_id", type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"] @name("meta.stage3_dash_acl_group_id");
            meta._dst_ip_addr29             : optional @SaiVal[name="dip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.dst_ip_addr");
            meta._src_ip_addr30             : optional @SaiVal[name="sip", type="sai_ip_prefix_list_t", match_type="list"] @name("meta.src_ip_addr");
            meta._ip_protocol28             : optional @SaiVal[name="protocol", type="sai_u8_list_t", match_type="list"] @name("meta.ip_protocol");
            meta._src_l4_port34             : optional @SaiVal[name="src_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.src_l4_port");
            meta._dst_l4_port35             : optional @SaiVal[name="dst_port", type="sai_u16_range_list_t", match_type="range_list"] @name("meta.dst_l4_port");
        }
        actions = {
            inbound_acl_permit_2();
            inbound_acl_permit_and_continue_2();
            inbound_acl_deny_2();
            inbound_acl_deny_and_continue_2();
        }
        default_action = inbound_acl_deny_2();
        counters = inbound_acl_stage3_counter;
    }
    @name("dash_ingress.underlay.pkt_act") action underlay_pkt_act_0(@name("packet_action") bit<9> packet_action, @name("next_hop_id") bit<9> next_hop_id) {
        if (packet_action == 9w0) {
            meta._dropped0 = true;
        } else if (packet_action == 9w1) {
            standard_metadata.egress_spec = next_hop_id;
        }
    }
    @name("dash_ingress.underlay.def_act") action underlay_def_act_0() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @SaiTable[name="route", api="route", api_type="underlay"] @name("dash_ingress.underlay.underlay_routing") table underlay_underlay_routing {
        key = {
            meta._dst_ip_addr29: lpm @SaiVal[name="destination"] @name("meta.dst_ip_addr");
        }
        actions = {
            underlay_pkt_act_0();
            @defaultonly underlay_def_act_0();
            @defaultonly NoAction_8();
        }
        default_action = NoAction_8();
    }
    @hidden action dashpipelinev1modelbmv2l914() {
        meta._encap_data_underlay_sip4 = hdr.ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l918() {
        tmp_0 = hdr.inner_ethernet.src_addr;
    }
    @hidden action dashpipelinev1modelbmv2l918_0() {
        tmp_0 = hdr.inner_ethernet.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l918_1() {
        meta._eni_addr13 = tmp_0;
    }
    @hidden action dashpipelinev1modelbmv2l935() {
        meta._ip_protocol28 = hdr.ipv6.next_header;
        meta._src_ip_addr30 = hdr.ipv6.src_addr;
        meta._dst_ip_addr29 = hdr.ipv6.dst_addr;
        meta._is_overlay_ip_v626 = 1w1;
    }
    @hidden action dashpipelinev1modelbmv2l940() {
        meta._ip_protocol28 = hdr.ipv4.protocol;
        meta._src_ip_addr30 = (bit<128>)hdr.ipv4.src_addr;
        meta._dst_ip_addr29 = (bit<128>)hdr.ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l930() {
        meta._is_overlay_ip_v626 = 1w0;
        meta._ip_protocol28 = 8w0;
        meta._dst_ip_addr29 = 128w0;
        meta._src_ip_addr30 = 128w0;
    }
    @hidden action dashpipelinev1modelbmv2l945() {
        meta._src_l4_port34 = hdr.tcp.src_port;
        meta._dst_l4_port35 = hdr.tcp.dst_port;
    }
    @hidden action dashpipelinev1modelbmv2l948() {
        meta._src_l4_port34 = hdr.udp.src_port;
        meta._dst_l4_port35 = hdr.udp.dst_port;
    }
    @hidden action dashpipelinev1modelbmv2l460() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l463() {
        outbound_acl_hasReturned = true;
    }
    @hidden action act() {
        outbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l470() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l473() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l618() {
        meta._lkup_dst_ip_addr31 = meta._dst_ip_addr29;
        meta._is_lkup_dst_ip_v627 = meta._is_overlay_ip_v626;
    }
    @hidden action dashpipelinev1modelbmv2l460_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l463_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_0() {
        inbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinev1modelbmv2l470_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinev1modelbmv2l473_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_1() {
        inbound_tmp = hdr.ethernet.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l961() {
        meta._dst_ip_addr29 = (bit<128>)hdr.ipv4.dst_addr;
    }
    @hidden action dashpipelinev1modelbmv2l969() {
        meta._meter_class47 = meta._policy_meter_class44;
    }
    @hidden action dashpipelinev1modelbmv2l971() {
        meta._meter_class47 = meta._route_meter_class45;
    }
    @hidden action dashpipelinev1modelbmv2l974() {
        meta._meter_class47 = meta._mapping_meter_class46;
    }
    @hidden action dashpipelinev1modelbmv2l979() {
        meter_bucket_outbound_0.count(meta._meter_bucket_index48);
    }
    @hidden action dashpipelinev1modelbmv2l981() {
        meter_bucket_inbound_0.count(meta._meter_bucket_index48);
    }
    @hidden table tbl_dashpipelinev1modelbmv2l914 {
        actions = {
            dashpipelinev1modelbmv2l914();
        }
        const default_action = dashpipelinev1modelbmv2l914();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l918 {
        actions = {
            dashpipelinev1modelbmv2l918();
        }
        const default_action = dashpipelinev1modelbmv2l918();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l918_0 {
        actions = {
            dashpipelinev1modelbmv2l918_0();
        }
        const default_action = dashpipelinev1modelbmv2l918_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l918_1 {
        actions = {
            dashpipelinev1modelbmv2l918_1();
        }
        const default_action = dashpipelinev1modelbmv2l918_1();
    }
    @hidden table tbl_vxlan_decap {
        actions = {
            vxlan_decap_2();
        }
        const default_action = vxlan_decap_2();
    }
    @hidden table tbl_vxlan_decap_0 {
        actions = {
            vxlan_decap_3();
        }
        const default_action = vxlan_decap_3();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l930 {
        actions = {
            dashpipelinev1modelbmv2l930();
        }
        const default_action = dashpipelinev1modelbmv2l930();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l935 {
        actions = {
            dashpipelinev1modelbmv2l935();
        }
        const default_action = dashpipelinev1modelbmv2l935();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l940 {
        actions = {
            dashpipelinev1modelbmv2l940();
        }
        const default_action = dashpipelinev1modelbmv2l940();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l945 {
        actions = {
            dashpipelinev1modelbmv2l945();
        }
        const default_action = dashpipelinev1modelbmv2l945();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l948 {
        actions = {
            dashpipelinev1modelbmv2l948();
        }
        const default_action = dashpipelinev1modelbmv2l948();
    }
    @hidden table tbl_deny {
        actions = {
            deny_5();
        }
        const default_action = deny_5();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l460 {
        actions = {
            dashpipelinev1modelbmv2l460();
        }
        const default_action = dashpipelinev1modelbmv2l460();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l463 {
        actions = {
            dashpipelinev1modelbmv2l463();
        }
        const default_action = dashpipelinev1modelbmv2l463();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l470 {
        actions = {
            dashpipelinev1modelbmv2l470();
        }
        const default_action = dashpipelinev1modelbmv2l470();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l473 {
        actions = {
            dashpipelinev1modelbmv2l473();
        }
        const default_action = dashpipelinev1modelbmv2l473();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l618 {
        actions = {
            dashpipelinev1modelbmv2l618();
        }
        const default_action = dashpipelinev1modelbmv2l618();
    }
    @hidden table tbl_vxlan_encap {
        actions = {
            vxlan_encap_1();
        }
        const default_action = vxlan_encap_1();
    }
    @hidden table tbl_nvgre_encap {
        actions = {
            nvgre_encap_1();
        }
        const default_action = nvgre_encap_1();
    }
    @hidden table tbl_outbound_drop {
        actions = {
            outbound_drop_2();
        }
        const default_action = outbound_drop_2();
    }
    @hidden table tbl_vxlan_encap_0 {
        actions = {
            vxlan_encap_2();
        }
        const default_action = vxlan_encap_2();
    }
    @hidden table tbl_nvgre_encap_0 {
        actions = {
            nvgre_encap_2();
        }
        const default_action = nvgre_encap_2();
    }
    @hidden table tbl_outbound_drop_0 {
        actions = {
            outbound_drop_3();
        }
        const default_action = outbound_drop_3();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l460_0 {
        actions = {
            dashpipelinev1modelbmv2l460_0();
        }
        const default_action = dashpipelinev1modelbmv2l460_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l463_0 {
        actions = {
            dashpipelinev1modelbmv2l463_0();
        }
        const default_action = dashpipelinev1modelbmv2l463_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l470_0 {
        actions = {
            dashpipelinev1modelbmv2l470_0();
        }
        const default_action = dashpipelinev1modelbmv2l470_0();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l473_0 {
        actions = {
            dashpipelinev1modelbmv2l473_0();
        }
        const default_action = dashpipelinev1modelbmv2l473_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_vxlan_encap_1 {
        actions = {
            vxlan_encap_3();
        }
        const default_action = vxlan_encap_3();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l961 {
        actions = {
            dashpipelinev1modelbmv2l961();
        }
        const default_action = dashpipelinev1modelbmv2l961();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l969 {
        actions = {
            dashpipelinev1modelbmv2l969();
        }
        const default_action = dashpipelinev1modelbmv2l969();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l971 {
        actions = {
            dashpipelinev1modelbmv2l971();
        }
        const default_action = dashpipelinev1modelbmv2l971();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l974 {
        actions = {
            dashpipelinev1modelbmv2l974();
        }
        const default_action = dashpipelinev1modelbmv2l974();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l979 {
        actions = {
            dashpipelinev1modelbmv2l979();
        }
        const default_action = dashpipelinev1modelbmv2l979();
    }
    @hidden table tbl_dashpipelinev1modelbmv2l981 {
        actions = {
            dashpipelinev1modelbmv2l981();
        }
        const default_action = dashpipelinev1modelbmv2l981();
    }
    @hidden table tbl_drop_action {
        actions = {
            drop_action();
        }
        const default_action = drop_action();
    }
    apply {
        if (vip_0.apply().hit) {
            tbl_dashpipelinev1modelbmv2l914.apply();
        }
        direction_lookup_0.apply();
        appliance_0.apply();
        if (meta._direction1 == 16w1) {
            tbl_dashpipelinev1modelbmv2l918.apply();
        } else {
            tbl_dashpipelinev1modelbmv2l918_0.apply();
        }
        tbl_dashpipelinev1modelbmv2l918_1.apply();
        eni_ether_address_map_0.apply();
        if (meta._direction1 == 16w1) {
            tbl_vxlan_decap.apply();
        } else if (meta._direction1 == 16w2) {
            switch (inbound_routing_0.apply().action_run) {
                vxlan_decap_pa_validate: {
                    pa_validation_0.apply();
                    tbl_vxlan_decap_0.apply();
                }
                default: {
                }
            }
        }
        tbl_dashpipelinev1modelbmv2l930.apply();
        if (hdr.ipv6.isValid()) {
            tbl_dashpipelinev1modelbmv2l935.apply();
        } else if (hdr.ipv4.isValid()) {
            tbl_dashpipelinev1modelbmv2l940.apply();
        }
        if (hdr.tcp.isValid()) {
            tbl_dashpipelinev1modelbmv2l945.apply();
        } else if (hdr.udp.isValid()) {
            tbl_dashpipelinev1modelbmv2l948.apply();
        }
        eni_0.apply();
        if (meta._eni_data_admin_state20 == 1w0) {
            tbl_deny.apply();
        }
        acl_group_0.apply();
        if (meta._direction1 == 16w1) {
            if (meta._conntrack_data_allow_out33) {
                ;
            } else {
                tbl_act.apply();
                if (meta._stage1_dash_acl_group_id36 != 16w0) {
                    switch (outbound_acl_stage1.apply().action_run) {
                        outbound_acl_permit_0: {
                            tbl_dashpipelinev1modelbmv2l460.apply();
                        }
                        outbound_acl_deny_0: {
                            tbl_dashpipelinev1modelbmv2l463.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id37 != 16w0) {
                    switch (outbound_acl_stage2.apply().action_run) {
                        outbound_acl_permit_1: {
                            tbl_dashpipelinev1modelbmv2l470.apply();
                        }
                        outbound_acl_deny_1: {
                            tbl_dashpipelinev1modelbmv2l473.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id38 != 16w0) {
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
            tbl_dashpipelinev1modelbmv2l618.apply();
            switch (outbound_routing.apply().action_run) {
                outbound_route_vnet_direct_0: 
                outbound_route_vnet_0: {
                    switch (outbound_ca_to_pa.apply().action_run) {
                        outbound_set_tunnel_mapping_0: {
                            outbound_vnet.apply();
                        }
                        default: {
                        }
                    }
                    if (meta._encap_data_dash_encapsulation9 == 16w1) {
                        tbl_vxlan_encap.apply();
                    } else if (meta._encap_data_dash_encapsulation9 == 16w2) {
                        tbl_nvgre_encap.apply();
                    } else {
                        tbl_outbound_drop.apply();
                    }
                }
                outbound_route_service_tunnel_0: {
                    if (meta._encap_data_dash_encapsulation9 == 16w1) {
                        tbl_vxlan_encap_0.apply();
                    } else if (meta._encap_data_dash_encapsulation9 == 16w2) {
                        tbl_nvgre_encap_0.apply();
                    } else {
                        tbl_outbound_drop_0.apply();
                    }
                }
                default: {
                }
            }
        } else if (meta._direction1 == 16w2) {
            if (meta._conntrack_data_allow_in32) {
                ;
            } else {
                tbl_act_0.apply();
                if (meta._stage1_dash_acl_group_id36 != 16w0) {
                    switch (inbound_acl_stage1.apply().action_run) {
                        inbound_acl_permit_0: {
                            tbl_dashpipelinev1modelbmv2l460_0.apply();
                        }
                        inbound_acl_deny_0: {
                            tbl_dashpipelinev1modelbmv2l463_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id37 != 16w0) {
                    switch (inbound_acl_stage2.apply().action_run) {
                        inbound_acl_permit_1: {
                            tbl_dashpipelinev1modelbmv2l470_0.apply();
                        }
                        inbound_acl_deny_1: {
                            tbl_dashpipelinev1modelbmv2l473_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id38 != 16w0) {
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
            tbl_act_1.apply();
            tbl_vxlan_encap_1.apply();
        }
        tbl_dashpipelinev1modelbmv2l961.apply();
        underlay_underlay_routing.apply();
        if (meta._meter_policy_en41 == 1w1) {
            meter_policy_0.apply();
            meter_rule_0.apply();
        }
        if (meta._meter_policy_en41 == 1w1) {
            tbl_dashpipelinev1modelbmv2l969.apply();
        } else {
            tbl_dashpipelinev1modelbmv2l971.apply();
        }
        if (meta._meter_class47 == 16w0 || meta._mapping_meter_class_override42 == 1w1) {
            tbl_dashpipelinev1modelbmv2l974.apply();
        }
        meter_bucket_0.apply();
        if (meta._direction1 == 16w1) {
            tbl_dashpipelinev1modelbmv2l979.apply();
        } else if (meta._direction1 == 16w2) {
            tbl_dashpipelinev1modelbmv2l981.apply();
        }
        eni_meter_0.apply();
        if (meta._dropped0) {
            tbl_drop_action.apply();
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
