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
    bit<32> cps;
    bit<32> pps;
    bit<32> flows;
    bit<1>  admin_state;
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
    bit<16>  _inbound_vm_id21;
    bit<8>   _appliance_id22;
    bit<1>   _is_overlay_ip_v623;
    bit<1>   _is_lkup_dst_ip_v624;
    bit<8>   _ip_protocol25;
    bit<128> _dst_ip_addr26;
    bit<128> _src_ip_addr27;
    bit<128> _lkup_dst_ip_addr28;
    bool     _conntrack_data_allow_in29;
    bool     _conntrack_data_allow_out30;
    bit<16>  _src_l4_port31;
    bit<16>  _dst_l4_port32;
    bit<16>  _stage1_dash_acl_group_id33;
    bit<16>  _stage2_dash_acl_group_id34;
    bit<16>  _stage3_dash_acl_group_id35;
    bit<16>  _stage4_dash_acl_group_id36;
    bit<16>  _stage5_dash_acl_group_id37;
    bit<1>   _meter_policy_en38;
    bit<1>   _mapping_meter_class_override39;
    bit<16>  _meter_policy_id40;
    bit<16>  _policy_meter_class41;
    bit<16>  _route_meter_class42;
    bit<16>  _mapping_meter_class43;
    bit<16>  _meter_class44;
    bit<32>  _meter_bucket_index45;
    bit<32>  _src_tag_map46;
    bit<32>  _dst_tag_map47;
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
    @name("dash_ingress.tmp_2") bit<48> tmp_0;
    @name("dash_ingress.outbound.tmp") bit<32> outbound_tmp;
    @name("dash_ingress.outbound.tmp_0") bit<32> outbound_tmp_0;
    ethernet_t hdr_0_ethernet;
    ipv4_t hdr_0_ipv4;
    ipv6_t hdr_0_ipv6;
    @name("dash_ingress.inbound.tmp_1") bit<48> inbound_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    udp_t hdr_1_udp;
    vxlan_t hdr_1_vxlan;
    ethernet_t hdr_1_inner_ethernet;
    ipv4_t hdr_1_inner_ipv4;
    ipv6_t hdr_1_inner_ipv6;
    udp_t hdr_1_inner_udp;
    tcp_t hdr_1_inner_tcp;
    udp_t hdr_2_udp;
    vxlan_t hdr_2_vxlan;
    ethernet_t hdr_2_inner_ethernet;
    ipv4_t hdr_2_inner_ipv4;
    ipv6_t hdr_2_inner_ipv6;
    udp_t hdr_2_inner_udp;
    tcp_t hdr_2_inner_tcp;
    udp_t hdr_11_udp;
    vxlan_t hdr_11_vxlan;
    ethernet_t hdr_11_inner_ethernet;
    ipv4_t hdr_11_inner_ipv4;
    ipv6_t hdr_11_inner_ipv6;
    udp_t hdr_11_inner_udp;
    tcp_t hdr_11_inner_tcp;
    ethernet_t hdr_12_ethernet;
    ipv4_t hdr_12_ipv4;
    ipv6_t hdr_12_ipv6;
    udp_t hdr_12_udp;
    tcp_t hdr_12_tcp;
    vxlan_t hdr_12_vxlan;
    ethernet_t hdr_12_inner_ethernet;
    ipv4_t hdr_12_inner_ipv4;
    ipv6_t hdr_12_inner_ipv6;
    udp_t hdr_12_inner_udp;
    tcp_t hdr_12_inner_tcp;
    ethernet_t hdr_13_ethernet;
    ipv4_t hdr_13_ipv4;
    ipv6_t hdr_13_ipv6;
    udp_t hdr_13_udp;
    tcp_t hdr_13_tcp;
    vxlan_t hdr_13_vxlan;
    ethernet_t hdr_13_inner_ethernet;
    ipv4_t hdr_13_inner_ipv4;
    ipv6_t hdr_13_inner_ipv6;
    udp_t hdr_13_inner_udp;
    tcp_t hdr_13_inner_tcp;
    ethernet_t hdr_14_ethernet;
    ipv4_t hdr_14_ipv4;
    ipv6_t hdr_14_ipv6;
    udp_t hdr_14_udp;
    tcp_t hdr_14_tcp;
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
    nvgre_t hdr_15_nvgre;
    ethernet_t hdr_15_inner_ethernet;
    ipv4_t hdr_15_inner_ipv4;
    ipv6_t hdr_15_inner_ipv6;
    udp_t hdr_15_inner_udp;
    tcp_t hdr_15_inner_tcp;
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
        hdr_1_udp = hdr.udp;
        hdr_1_vxlan = hdr.vxlan;
        hdr_1_inner_ethernet = hdr.inner_ethernet;
        hdr_1_inner_ipv4 = hdr.inner_ipv4;
        hdr_1_inner_ipv6 = hdr.inner_ipv6;
        hdr_1_inner_udp = hdr.inner_udp;
        hdr_1_inner_tcp = hdr.inner_tcp;
        hdr_1_inner_ethernet.setInvalid();
        hdr_1_inner_ipv4.setInvalid();
        hdr_1_inner_ipv6.setInvalid();
        hdr_1_vxlan.setInvalid();
        hdr_1_udp.setInvalid();
        hdr_1_inner_tcp.setInvalid();
        hdr_1_udp = hdr.inner_udp;
        hdr_1_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_1_vxlan;
        hdr.inner_ethernet = hdr_1_inner_ethernet;
        hdr.inner_ipv4 = hdr_1_inner_ipv4;
        hdr.inner_ipv6 = hdr_1_inner_ipv6;
        hdr.inner_udp = hdr_1_inner_udp;
        hdr.inner_tcp = hdr_1_inner_tcp;
    }
    @name(".vxlan_decap") action vxlan_decap_2() {
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
    @name(".vxlan_decap") action vxlan_decap_3() {
        hdr_11_udp = hdr.udp;
        hdr_11_vxlan = hdr.vxlan;
        hdr_11_inner_ethernet = hdr.inner_ethernet;
        hdr_11_inner_ipv4 = hdr.inner_ipv4;
        hdr_11_inner_ipv6 = hdr.inner_ipv6;
        hdr_11_inner_udp = hdr.inner_udp;
        hdr_11_inner_tcp = hdr.inner_tcp;
        hdr_11_inner_ethernet.setInvalid();
        hdr_11_inner_ipv4.setInvalid();
        hdr_11_inner_ipv6.setInvalid();
        hdr_11_vxlan.setInvalid();
        hdr_11_udp.setInvalid();
        hdr_11_inner_tcp.setInvalid();
        hdr_11_udp = hdr.inner_udp;
        hdr_11_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_11_vxlan;
        hdr.inner_ethernet = hdr_11_inner_ethernet;
        hdr.inner_ipv4 = hdr_11_inner_ipv4;
        hdr.inner_ipv6 = hdr_11_inner_ipv6;
        hdr.inner_udp = hdr_11_inner_udp;
        hdr.inner_tcp = hdr_11_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_1() {
        hdr_12_ethernet = hdr.ethernet;
        hdr_12_ipv4 = hdr.ipv4;
        hdr_12_ipv6 = hdr.ipv6;
        hdr_12_udp = hdr.udp;
        hdr_12_tcp = hdr.tcp;
        hdr_12_vxlan = hdr.vxlan;
        hdr_12_inner_ethernet = hdr.inner_ethernet;
        hdr_12_inner_ipv4 = hdr.inner_ipv4;
        hdr_12_inner_ipv6 = hdr.inner_ipv6;
        hdr_12_inner_udp = hdr.inner_udp;
        hdr_12_inner_tcp = hdr.inner_tcp;
        hdr_12_inner_ethernet = hdr.ethernet;
        hdr_12_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_12_ethernet.setInvalid();
        hdr_12_inner_ipv4 = hdr.ipv4;
        hdr_12_ipv4.setInvalid();
        hdr_12_inner_ipv6 = hdr.ipv6;
        hdr_12_ipv6.setInvalid();
        hdr_12_inner_tcp = hdr.tcp;
        hdr_12_tcp.setInvalid();
        hdr_12_inner_udp = hdr.udp;
        hdr_12_udp.setInvalid();
        hdr_12_ethernet.setValid();
        hdr_12_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_12_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_12_ethernet.ether_type = 16w0x800;
        hdr_12_ipv4.setValid();
        hdr_12_ipv4.version = 4w4;
        hdr_12_ipv4.ihl = 4w5;
        hdr_12_ipv4.diffserv = 8w0;
        hdr_12_ipv4.total_len = hdr_12_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_12_inner_ipv4.isValid() + hdr_12_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_12_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_12_inner_ipv6.isValid() + 16w50;
        hdr_12_ipv4.identification = 16w1;
        hdr_12_ipv4.flags = 3w0;
        hdr_12_ipv4.frag_offset = 13w0;
        hdr_12_ipv4.ttl = 8w64;
        hdr_12_ipv4.protocol = 8w17;
        hdr_12_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_12_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_12_ipv4.hdr_checksum = 16w0;
        hdr_12_udp.setValid();
        hdr_12_udp.src_port = 16w0;
        hdr_12_udp.dst_port = 16w4789;
        hdr_12_udp.length = hdr_12_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_12_inner_ipv4.isValid() + hdr_12_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_12_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_12_inner_ipv6.isValid() + 16w30;
        hdr_12_udp.checksum = 16w0;
        hdr_12_vxlan.setValid();
        hdr_12_vxlan.reserved = 24w0;
        hdr_12_vxlan.reserved_2 = 8w0;
        hdr_12_vxlan.flags = 8w0;
        hdr_12_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_12_ethernet;
        hdr.ipv4 = hdr_12_ipv4;
        hdr.ipv6 = hdr_12_ipv6;
        hdr.udp = hdr_12_udp;
        hdr.tcp = hdr_12_tcp;
        hdr.vxlan = hdr_12_vxlan;
        hdr.inner_ethernet = hdr_12_inner_ethernet;
        hdr.inner_ipv4 = hdr_12_inner_ipv4;
        hdr.inner_ipv6 = hdr_12_inner_ipv6;
        hdr.inner_udp = hdr_12_inner_udp;
        hdr.inner_tcp = hdr_12_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_2() {
        hdr_13_ethernet = hdr.ethernet;
        hdr_13_ipv4 = hdr.ipv4;
        hdr_13_ipv6 = hdr.ipv6;
        hdr_13_udp = hdr.udp;
        hdr_13_tcp = hdr.tcp;
        hdr_13_vxlan = hdr.vxlan;
        hdr_13_inner_ethernet = hdr.inner_ethernet;
        hdr_13_inner_ipv4 = hdr.inner_ipv4;
        hdr_13_inner_ipv6 = hdr.inner_ipv6;
        hdr_13_inner_udp = hdr.inner_udp;
        hdr_13_inner_tcp = hdr.inner_tcp;
        hdr_13_inner_ethernet = hdr.ethernet;
        hdr_13_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_13_ethernet.setInvalid();
        hdr_13_inner_ipv4 = hdr.ipv4;
        hdr_13_ipv4.setInvalid();
        hdr_13_inner_ipv6 = hdr.ipv6;
        hdr_13_ipv6.setInvalid();
        hdr_13_inner_tcp = hdr.tcp;
        hdr_13_tcp.setInvalid();
        hdr_13_inner_udp = hdr.udp;
        hdr_13_udp.setInvalid();
        hdr_13_ethernet.setValid();
        hdr_13_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_13_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_13_ethernet.ether_type = 16w0x800;
        hdr_13_ipv4.setValid();
        hdr_13_ipv4.version = 4w4;
        hdr_13_ipv4.ihl = 4w5;
        hdr_13_ipv4.diffserv = 8w0;
        hdr_13_ipv4.total_len = hdr_13_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_13_inner_ipv4.isValid() + hdr_13_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_13_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_13_inner_ipv6.isValid() + 16w50;
        hdr_13_ipv4.identification = 16w1;
        hdr_13_ipv4.flags = 3w0;
        hdr_13_ipv4.frag_offset = 13w0;
        hdr_13_ipv4.ttl = 8w64;
        hdr_13_ipv4.protocol = 8w17;
        hdr_13_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_13_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_13_ipv4.hdr_checksum = 16w0;
        hdr_13_udp.setValid();
        hdr_13_udp.src_port = 16w0;
        hdr_13_udp.dst_port = 16w4789;
        hdr_13_udp.length = hdr_13_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_13_inner_ipv4.isValid() + hdr_13_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_13_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_13_inner_ipv6.isValid() + 16w30;
        hdr_13_udp.checksum = 16w0;
        hdr_13_vxlan.setValid();
        hdr_13_vxlan.reserved = 24w0;
        hdr_13_vxlan.reserved_2 = 8w0;
        hdr_13_vxlan.flags = 8w0;
        hdr_13_vxlan.vni = meta._encap_data_service_tunnel_key10;
        hdr.ethernet = hdr_13_ethernet;
        hdr.ipv4 = hdr_13_ipv4;
        hdr.ipv6 = hdr_13_ipv6;
        hdr.udp = hdr_13_udp;
        hdr.tcp = hdr_13_tcp;
        hdr.vxlan = hdr_13_vxlan;
        hdr.inner_ethernet = hdr_13_inner_ethernet;
        hdr.inner_ipv4 = hdr_13_inner_ipv4;
        hdr.inner_ipv6 = hdr_13_inner_ipv6;
        hdr.inner_udp = hdr_13_inner_udp;
        hdr.inner_tcp = hdr_13_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_3() {
        hdr_14_ethernet = hdr.ethernet;
        hdr_14_ipv4 = hdr.ipv4;
        hdr_14_ipv6 = hdr.ipv6;
        hdr_14_udp = hdr.udp;
        hdr_14_tcp = hdr.tcp;
        hdr_14_vxlan = hdr.vxlan;
        hdr_14_inner_ethernet = hdr.inner_ethernet;
        hdr_14_inner_ipv4 = hdr.inner_ipv4;
        hdr_14_inner_ipv6 = hdr.inner_ipv6;
        hdr_14_inner_udp = hdr.inner_udp;
        hdr_14_inner_tcp = hdr.inner_tcp;
        hdr_14_inner_ethernet = hdr.ethernet;
        hdr_14_inner_ethernet.dst_addr = inbound_tmp;
        hdr_14_ethernet.setInvalid();
        hdr_14_inner_ipv4 = hdr.ipv4;
        hdr_14_ipv4.setInvalid();
        hdr_14_inner_ipv6 = hdr.ipv6;
        hdr_14_ipv6.setInvalid();
        hdr_14_inner_tcp = hdr.tcp;
        hdr_14_tcp.setInvalid();
        hdr_14_inner_udp = hdr.udp;
        hdr_14_udp.setInvalid();
        hdr_14_ethernet.setValid();
        hdr_14_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_14_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_14_ethernet.ether_type = 16w0x800;
        hdr_14_ipv4.setValid();
        hdr_14_ipv4.version = 4w4;
        hdr_14_ipv4.ihl = 4w5;
        hdr_14_ipv4.diffserv = 8w0;
        hdr_14_ipv4.total_len = hdr_14_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_14_inner_ipv4.isValid() + hdr_14_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_14_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_14_inner_ipv6.isValid() + 16w50;
        hdr_14_ipv4.identification = 16w1;
        hdr_14_ipv4.flags = 3w0;
        hdr_14_ipv4.frag_offset = 13w0;
        hdr_14_ipv4.ttl = 8w64;
        hdr_14_ipv4.protocol = 8w17;
        hdr_14_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_14_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_14_ipv4.hdr_checksum = 16w0;
        hdr_14_udp.setValid();
        hdr_14_udp.src_port = 16w0;
        hdr_14_udp.dst_port = 16w4789;
        hdr_14_udp.length = hdr_14_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_14_inner_ipv4.isValid() + hdr_14_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_14_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_14_inner_ipv6.isValid() + 16w30;
        hdr_14_udp.checksum = 16w0;
        hdr_14_vxlan.setValid();
        hdr_14_vxlan.reserved = 24w0;
        hdr_14_vxlan.reserved_2 = 8w0;
        hdr_14_vxlan.flags = 8w0;
        hdr_14_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_14_ethernet;
        hdr.ipv4 = hdr_14_ipv4;
        hdr.ipv6 = hdr_14_ipv6;
        hdr.udp = hdr_14_udp;
        hdr.tcp = hdr_14_tcp;
        hdr.vxlan = hdr_14_vxlan;
        hdr.inner_ethernet = hdr_14_inner_ethernet;
        hdr.inner_ipv4 = hdr_14_inner_ipv4;
        hdr.inner_ipv6 = hdr_14_inner_ipv6;
        hdr.inner_udp = hdr_14_inner_udp;
        hdr.inner_tcp = hdr_14_inner_tcp;
    }
    @name(".nvgre_encap") action nvgre_encap_0() {
        hdr_15_ethernet = hdr.ethernet;
        hdr_15_ipv4 = hdr.ipv4;
        hdr_15_ipv6 = hdr.ipv6;
        hdr_15_udp = hdr.udp;
        hdr_15_tcp = hdr.tcp;
        hdr_15_nvgre = hdr.nvgre;
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
        hdr_15_ipv4.total_len = hdr_15_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_15_inner_ipv4.isValid() + hdr_15_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_15_inner_ipv6.isValid() + 16w42;
        hdr_15_ipv4.identification = 16w1;
        hdr_15_ipv4.flags = 3w0;
        hdr_15_ipv4.frag_offset = 13w0;
        hdr_15_ipv4.ttl = 8w64;
        hdr_15_ipv4.protocol = 8w0x2f;
        hdr_15_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_15_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_15_ipv4.hdr_checksum = 16w0;
        hdr_15_nvgre.setValid();
        hdr_15_nvgre.flags = 4w4;
        hdr_15_nvgre.reserved = 9w0;
        hdr_15_nvgre.version = 3w0;
        hdr_15_nvgre.protocol_type = 16w0x6558;
        hdr_15_nvgre.vsid = meta._encap_data_service_tunnel_key10;
        hdr_15_nvgre.flow_id = 8w0;
        hdr.ethernet = hdr_15_ethernet;
        hdr.ipv4 = hdr_15_ipv4;
        hdr.ipv6 = hdr_15_ipv6;
        hdr.udp = hdr_15_udp;
        hdr.tcp = hdr_15_tcp;
        hdr.nvgre = hdr_15_nvgre;
        hdr.inner_ethernet = hdr_15_inner_ethernet;
        hdr.inner_ipv4 = hdr_15_inner_ipv4;
        hdr.inner_ipv6 = hdr_15_inner_ipv6;
        hdr.inner_udp = hdr_15_inner_udp;
        hdr.inner_tcp = hdr_15_inner_tcp;
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
        meta._direction1 = 16w1;
    }
    @name("dash_ingress.set_inbound_direction") action set_inbound_direction() {
        meta._direction1 = 16w2;
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
    @name("dash_ingress.set_appliance") action set_appliance(@name("neighbor_mac") bit<48> neighbor_mac, @name("mac") bit<48> mac) {
        meta._encap_data_underlay_dmac7 = neighbor_mac;
        meta._encap_data_underlay_smac6 = mac;
    }
    @name("dash_ingress.appliance") table appliance_0 {
        key = {
            meta._appliance_id22: ternary @name("meta.appliance_id:appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @name("vm_underlay_dip") bit<32> vm_underlay_dip, @Sai[type="sai_uint32_t"] @name("vm_vni") bit<24> vm_vni, @name("vnet_id") bit<16> vnet_id_1, @name("v4_meter_policy_id") bit<16> v4_meter_policy_id, @name("v6_meter_policy_id") bit<16> v6_meter_policy_id, @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta._eni_data_cps17 = cps_1;
        meta._eni_data_pps18 = pps_1;
        meta._eni_data_flows19 = flows_1;
        meta._eni_data_admin_state20 = admin_state_1;
        meta._encap_data_underlay_dip5 = vm_underlay_dip;
        meta._encap_data_vni2 = vm_vni;
        meta._vnet_id14 = vnet_id_1;
        if (meta._is_overlay_ip_v623 == 1w1) {
            if (meta._direction1 == 16w1) {
                meta._stage1_dash_acl_group_id33 = outbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id34 = outbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id35 = outbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id36 = outbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id37 = outbound_v6_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id33 = inbound_v6_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id34 = inbound_v6_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id35 = inbound_v6_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id36 = inbound_v6_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id37 = inbound_v6_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id40 = v6_meter_policy_id;
        } else {
            if (meta._direction1 == 16w1) {
                meta._stage1_dash_acl_group_id33 = outbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id34 = outbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id35 = outbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id36 = outbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id37 = outbound_v4_stage5_dash_acl_group_id;
            } else {
                meta._stage1_dash_acl_group_id33 = inbound_v4_stage1_dash_acl_group_id;
                meta._stage2_dash_acl_group_id34 = inbound_v4_stage2_dash_acl_group_id;
                meta._stage3_dash_acl_group_id35 = inbound_v4_stage3_dash_acl_group_id;
                meta._stage4_dash_acl_group_id36 = inbound_v4_stage4_dash_acl_group_id;
                meta._stage5_dash_acl_group_id37 = inbound_v4_stage5_dash_acl_group_id;
            }
            meta._meter_policy_id40 = v4_meter_policy_id;
        }
    }
    @name("dash_ingress.eni|dash_eni") table eni_0 {
        key = {
            meta._eni_id16: exact @name("meta.eni_id:eni_id");
        }
        actions = {
            set_eni_attrs();
            @defaultonly deny_0();
        }
        const default_action = deny_0();
    }
    @name("dash_ingress.eni_counter") direct_counter(CounterType.packets_and_bytes) eni_counter_0;
    @name("dash_ingress.eni_meter") table eni_meter_0 {
        key = {
            meta._eni_id16  : exact @name("meta.eni_id:eni_id");
            meta._direction1: exact @name("meta.direction:direction");
            meta._dropped0  : exact @name("meta.dropped:dropped");
        }
        actions = {
            NoAction_2();
        }
        counters = eni_counter_0;
        default_action = NoAction_2();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.vxlan_decap_pa_validate") action vxlan_decap_pa_validate(@name("src_vnet_id") bit<16> src_vnet_id) {
        meta._vnet_id14 = src_vnet_id;
    }
    @name("dash_ingress.pa_validation|dash_pa_validation") table pa_validation_0 {
        key = {
            meta._vnet_id14  : exact @name("meta.vnet_id:vnet_id");
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
            meta._eni_id16   : exact @name("meta.eni_id:eni_id");
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
            if (meta._is_overlay_ip_v623 == 1w1) {
                meta._dropped0 = true;
            }
        } else if (meta._is_overlay_ip_v623 == 1w0) {
            meta._dropped0 = true;
        }
    }
    @name("dash_ingress.meter_policy|dash_meter") @Sai[isobject="true"] table meter_policy_0 {
        key = {
            meta._meter_policy_id40: exact @name("meta.meter_policy_id:meter_policy_id");
        }
        actions = {
            check_ip_addr_family();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("dash_ingress.set_policy_meter_class") action set_policy_meter_class(@name("meter_class") bit<16> meter_class_0) {
        meta._policy_meter_class41 = meter_class_0;
    }
    @name("dash_ingress.meter_rule|dash_meter") @Sai[isobject="true"] table meter_rule_0 {
        key = {
            meta._meter_policy_id40: exact @name("meta.meter_policy_id:meter_policy_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"];
            hdr.ipv4.dst_addr      : ternary @name("hdr.ipv4.dst_addr:dip");
        }
        actions = {
            set_policy_meter_class();
            @defaultonly NoAction_4();
        }
        const default_action = NoAction_4();
    }
    @name("dash_ingress.meter_bucket_inbound") counter(32w262144, CounterType.bytes) meter_bucket_inbound_0;
    @name("dash_ingress.meter_bucket_outbound") counter(32w262144, CounterType.bytes) meter_bucket_outbound_0;
    @name("dash_ingress.meter_bucket_action") action meter_bucket_action(@Sai[type="sai_uint64_t", isreadonly="true"] @name("outbound_bytes_counter") bit<64> outbound_bytes_counter, @Sai[type="sai_uint64_t", isreadonly="true"] @name("inbound_bytes_counter") bit<64> inbound_bytes_counter, @Sai[type="sai_uint32_t", skipattr="true"] @name("meter_bucket_index") bit<32> meter_bucket_index_1) {
        meta._meter_bucket_index45 = meter_bucket_index_1;
    }
    @name("dash_ingress.meter_bucket|dash_meter") @Sai[isobject="true"] table meter_bucket_0 {
        key = {
            meta._eni_id16     : exact @name("meta.eni_id:eni_id");
            meta._meter_class44: exact @name("meta.meter_class:meter_class");
        }
        actions = {
            meter_bucket_action();
            @defaultonly NoAction_5();
        }
        const default_action = NoAction_5();
    }
    @name("dash_ingress.set_eni") action set_eni(@name("eni_id") bit<16> eni_id_1) {
        meta._eni_id16 = eni_id_1;
    }
    @name("dash_ingress.eni_ether_address_map|dash_eni") table eni_ether_address_map_0 {
        key = {
            tmp_0: exact @name("meta.eni_addr:address");
        }
        actions = {
            set_eni();
            @defaultonly deny_4();
        }
        const default_action = deny_4();
    }
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@Sai[type="sai_ip_addr_family_t", isresourcetype="true"] @name("ip_addr_family") bit<32> ip_addr_family_2) {
        if (ip_addr_family_2 == 32w0) {
            if (meta._is_overlay_ip_v623 == 1w1) {
                meta._dropped0 = true;
            }
        } else if (meta._is_overlay_ip_v623 == 1w0) {
            meta._dropped0 = true;
        }
    }
    @name("dash_ingress.dash_acl_group|dash_acl") table acl_group_0 {
        key = {
            meta._stage1_dash_acl_group_id33: exact @name("meta.stage1_dash_acl_group_id:dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_6();
        }
        default_action = NoAction_6();
    }
    @name("dash_ingress.set_src_tag") action set_src_tag(@name("tag_map") bit<32> tag_map) {
        meta._src_tag_map46 = tag_map;
    }
    @name("dash_ingress.src_tag|dash_tag") table src_tag_0 {
        key = {
            meta._src_ip_addr27: lpm @name("meta.src_ip_addr:sip");
        }
        actions = {
            set_src_tag();
            @defaultonly NoAction_7();
        }
        default_action = NoAction_7();
    }
    @name("dash_ingress.set_dst_tag") action set_dst_tag(@name("tag_map") bit<32> tag_map_2) {
        meta._dst_tag_map47 = tag_map_2;
    }
    @name("dash_ingress.dst_tag|dash_tag") table dst_tag_0 {
        key = {
            meta._dst_ip_addr26: lpm @name("meta.dst_ip_addr:dip");
        }
        actions = {
            set_dst_tag();
            @defaultonly NoAction_8();
        }
        default_action = NoAction_8();
    }
    @name("dash_ingress.outbound.route_vnet") action outbound_route_vnet_0(@name("dst_vnet_id") bit<16> dst_vnet_id_2, @name("meter_policy_en") bit<1> meter_policy_en_0, @name("meter_class") bit<16> meter_class_7) {
        meta._dst_vnet_id15 = dst_vnet_id_2;
        meta._meter_policy_en38 = meter_policy_en_0;
        meta._route_meter_class42 = meter_class_7;
    }
    @name("dash_ingress.outbound.route_vnet_direct") action outbound_route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("is_overlay_ip_v4_or_v6") bit<1> is_overlay_ip_v4_or_v6, @name("overlay_ip") bit<128> overlay_ip, @name("meter_policy_en") bit<1> meter_policy_en_5, @name("meter_class") bit<16> meter_class_8) {
        meta._dst_vnet_id15 = dst_vnet_id_3;
        meta._lkup_dst_ip_addr28 = overlay_ip;
        meta._is_lkup_dst_ip_v624 = is_overlay_ip_v4_or_v6;
        meta._meter_policy_en38 = meter_policy_en_5;
        meta._route_meter_class42 = meter_class_8;
    }
    @name("dash_ingress.outbound.route_direct") action outbound_route_direct_0(@name("meter_policy_en") bit<1> meter_policy_en_6, @name("meter_class") bit<16> meter_class_9) {
        meta._meter_policy_en38 = meter_policy_en_6;
        meta._route_meter_class42 = meter_class_9;
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
    @name("dash_ingress.outbound.route_service_tunnel") action outbound_route_service_tunnel_0(@name("is_overlay_dip_v4_or_v6") bit<1> is_overlay_dip_v4_or_v6, @name("overlay_dip") bit<128> overlay_dip, @name("is_overlay_dip_mask_v4_or_v6") bit<1> is_overlay_dip_mask_v4_or_v6, @name("overlay_dip_mask") bit<128> overlay_dip_mask, @name("is_overlay_sip_v4_or_v6") bit<1> is_overlay_sip_v4_or_v6, @name("overlay_sip") bit<128> overlay_sip, @name("is_overlay_sip_mask_v4_or_v6") bit<1> is_overlay_sip_mask_v4_or_v6, @name("overlay_sip_mask") bit<128> overlay_sip_mask, @name("is_underlay_dip_v4_or_v6") bit<1> is_underlay_dip_v4_or_v6, @name("underlay_dip") bit<128> underlay_dip_8, @name("is_underlay_sip_v4_or_v6") bit<1> is_underlay_sip_v4_or_v6, @name("underlay_sip") bit<128> underlay_sip_7, @name("dash_encapsulation") bit<16> dash_encapsulation_1, @name("tunnel_key") bit<24> tunnel_key, @name("meter_policy_en") bit<1> meter_policy_en_7, @name("meter_class") bit<16> meter_class_10) {
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
        if (underlay_dip_8 == 128w0) {
            outbound_tmp = meta._encap_data_original_overlay_dip12;
        } else {
            outbound_tmp = (bit<32>)underlay_dip_8;
        }
        meta._encap_data_underlay_dip5 = outbound_tmp;
        if (underlay_sip_7 == 128w0) {
            outbound_tmp_0 = meta._encap_data_original_overlay_sip11;
        } else {
            outbound_tmp_0 = (bit<32>)underlay_sip_7;
        }
        meta._encap_data_underlay_sip4 = outbound_tmp_0;
        meta._encap_data_overlay_dmac8 = hdr.ethernet.dst_addr;
        meta._encap_data_dash_encapsulation9 = dash_encapsulation_1;
        meta._encap_data_service_tunnel_key10 = tunnel_key;
        meta._meter_policy_en38 = meter_policy_en_7;
        meta._route_meter_class42 = meter_class_10;
    }
    @name("dash_ingress.outbound.routing_counter") direct_counter(CounterType.packets_and_bytes) outbound_routing_counter;
    @name("dash_ingress.outbound.outbound_routing|dash_outbound_routing") table outbound_outbound_routing_dash_outbound_routing {
        key = {
            meta._eni_id16          : exact @name("meta.eni_id:eni_id");
            meta._is_overlay_ip_v623: exact @name("meta.is_overlay_ip_v6:is_destination_v4_or_v6");
            meta._dst_ip_addr26     : lpm @name("meta.dst_ip_addr:destination");
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
    @name("dash_ingress.outbound.set_tunnel_mapping") action outbound_set_tunnel_mapping_0(@name("underlay_dip") bit<32> underlay_dip_9, @name("overlay_dmac") bit<48> overlay_dmac_7, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni, @name("meter_class") bit<16> meter_class_11, @name("meter_class_override") bit<1> meter_class_override) {
        if (use_dst_vnet_vni == 1w1) {
            meta._vnet_id14 = meta._dst_vnet_id15;
        }
        meta._encap_data_overlay_dmac8 = overlay_dmac_7;
        meta._encap_data_underlay_dip5 = underlay_dip_9;
        meta._mapping_meter_class43 = meter_class_11;
        meta._mapping_meter_class_override39 = meter_class_override;
    }
    @name("dash_ingress.outbound.ca_to_pa_counter") direct_counter(CounterType.packets_and_bytes) outbound_ca_to_pa_counter;
    @name("dash_ingress.outbound.outbound_ca_to_pa|dash_outbound_ca_to_pa") table outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa {
        key = {
            meta._dst_vnet_id15      : exact @name("meta.dst_vnet_id:dst_vnet_id");
            meta._is_lkup_dst_ip_v624: exact @name("meta.is_lkup_dst_ip_v6:is_dip_v4_or_v6");
            meta._lkup_dst_ip_addr28 : exact @name("meta.lkup_dst_ip_addr:dip");
        }
        actions = {
            outbound_set_tunnel_mapping_0();
            @defaultonly outbound_drop_1();
        }
        const default_action = outbound_drop_1();
        counters = outbound_ca_to_pa_counter;
    }
    @name("dash_ingress.outbound.set_vnet_attrs") action outbound_set_vnet_attrs_0(@name("vni") bit<24> vni_5) {
        meta._encap_data_vni2 = vni_5;
    }
    @name("dash_ingress.outbound.vnet|dash_vnet") table outbound_vnet_dash_vnet {
        key = {
            meta._vnet_id14: exact @name("meta.vnet_id:vnet_id");
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
    @name("dash_ingress.outbound.acl.stage1:dash_acl_rule|dash_acl") table outbound_acl_stage1_dash_acl_rule_dash_acl {
        key = {
            meta._stage1_dash_acl_group_id33: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.outbound.acl.stage2:dash_acl_rule|dash_acl") table outbound_acl_stage2_dash_acl_rule_dash_acl {
        key = {
            meta._stage2_dash_acl_group_id34: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.outbound.acl.stage3:dash_acl_rule|dash_acl") table outbound_acl_stage3_dash_acl_rule_dash_acl {
        key = {
            meta._stage3_dash_acl_group_id35: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.inbound.acl.stage1:dash_acl_rule|dash_acl") table inbound_acl_stage1_dash_acl_rule_dash_acl {
        key = {
            meta._stage1_dash_acl_group_id33: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.inbound.acl.stage2:dash_acl_rule|dash_acl") table inbound_acl_stage2_dash_acl_rule_dash_acl {
        key = {
            meta._stage2_dash_acl_group_id34: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.inbound.acl.stage3:dash_acl_rule|dash_acl") table inbound_acl_stage3_dash_acl_rule_dash_acl {
        key = {
            meta._stage3_dash_acl_group_id35: exact @name("meta.dash_acl_group_id:dash_acl_group_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="SAI_OBJECT_TYPE_DASH_ACL_GROUP"];
            meta._dst_tag_map47             : ternary @name("meta.dst_tag_map:dst_tag");
            meta._src_tag_map46             : ternary @name("meta.src_tag_map:src_tag");
            meta._dst_ip_addr26             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr27             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol25             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port31             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port32             : optional @name("meta.dst_l4_port:dst_port");
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
    @hidden action dashpipelinebmv2l892() {
        meta._encap_data_underlay_sip4 = hdr.ipv4.dst_addr;
    }
    @hidden action dashpipelinebmv2l890() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @hidden action dashpipelinebmv2l911() {
        meta._ip_protocol25 = hdr.ipv6.next_header;
        meta._src_ip_addr27 = hdr.ipv6.src_addr;
        meta._dst_ip_addr26 = hdr.ipv6.dst_addr;
        meta._is_overlay_ip_v623 = 1w1;
    }
    @hidden action dashpipelinebmv2l916() {
        meta._ip_protocol25 = hdr.ipv4.protocol;
        meta._src_ip_addr27 = (bit<128>)hdr.ipv4.src_addr;
        meta._dst_ip_addr26 = (bit<128>)hdr.ipv4.dst_addr;
    }
    @hidden action dashpipelinebmv2l906() {
        meta._is_overlay_ip_v623 = 1w0;
        meta._ip_protocol25 = 8w0;
        meta._dst_ip_addr26 = 128w0;
        meta._src_ip_addr27 = 128w0;
    }
    @hidden action dashpipelinebmv2l921() {
        meta._src_l4_port31 = hdr.tcp.src_port;
        meta._dst_l4_port32 = hdr.tcp.dst_port;
    }
    @hidden action dashpipelinebmv2l924() {
        meta._src_l4_port31 = hdr.udp.src_port;
        meta._dst_l4_port32 = hdr.udp.dst_port;
    }
    @hidden action dashpipelinebmv2l927() {
        tmp_0 = hdr.ethernet.src_addr;
    }
    @hidden action dashpipelinebmv2l927_0() {
        tmp_0 = hdr.ethernet.dst_addr;
    }
    @hidden action dashpipelinebmv2l927_1() {
        meta._eni_addr13 = tmp_0;
    }
    @hidden action dashpipelinebmv2l466() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinebmv2l469() {
        outbound_acl_hasReturned = true;
    }
    @hidden action act() {
        outbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinebmv2l476() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinebmv2l479() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinebmv2l613() {
        meta._lkup_dst_ip_addr28 = meta._dst_ip_addr26;
        meta._is_lkup_dst_ip_v624 = meta._is_overlay_ip_v623;
    }
    @hidden action dashpipelinebmv2l466_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinebmv2l469_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_0() {
        inbound_acl_hasReturned = false;
    }
    @hidden action dashpipelinebmv2l476_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipelinebmv2l479_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_1() {
        inbound_tmp = hdr.ethernet.dst_addr;
    }
    @hidden action dashpipelinebmv2l947() {
        meta._meter_class44 = meta._policy_meter_class41;
    }
    @hidden action dashpipelinebmv2l949() {
        meta._meter_class44 = meta._route_meter_class42;
    }
    @hidden action dashpipelinebmv2l952() {
        meta._meter_class44 = meta._mapping_meter_class43;
    }
    @hidden action dashpipelinebmv2l957() {
        meter_bucket_outbound_0.count(meta._meter_bucket_index45);
    }
    @hidden action dashpipelinebmv2l959() {
        meter_bucket_inbound_0.count(meta._meter_bucket_index45);
    }
    @hidden table tbl_dashpipelinebmv2l890 {
        actions = {
            dashpipelinebmv2l890();
        }
        const default_action = dashpipelinebmv2l890();
    }
    @hidden table tbl_dashpipelinebmv2l892 {
        actions = {
            dashpipelinebmv2l892();
        }
        const default_action = dashpipelinebmv2l892();
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
    @hidden table tbl_dashpipelinebmv2l906 {
        actions = {
            dashpipelinebmv2l906();
        }
        const default_action = dashpipelinebmv2l906();
    }
    @hidden table tbl_dashpipelinebmv2l911 {
        actions = {
            dashpipelinebmv2l911();
        }
        const default_action = dashpipelinebmv2l911();
    }
    @hidden table tbl_dashpipelinebmv2l916 {
        actions = {
            dashpipelinebmv2l916();
        }
        const default_action = dashpipelinebmv2l916();
    }
    @hidden table tbl_dashpipelinebmv2l921 {
        actions = {
            dashpipelinebmv2l921();
        }
        const default_action = dashpipelinebmv2l921();
    }
    @hidden table tbl_dashpipelinebmv2l924 {
        actions = {
            dashpipelinebmv2l924();
        }
        const default_action = dashpipelinebmv2l924();
    }
    @hidden table tbl_dashpipelinebmv2l927 {
        actions = {
            dashpipelinebmv2l927();
        }
        const default_action = dashpipelinebmv2l927();
    }
    @hidden table tbl_dashpipelinebmv2l927_0 {
        actions = {
            dashpipelinebmv2l927_0();
        }
        const default_action = dashpipelinebmv2l927_0();
    }
    @hidden table tbl_dashpipelinebmv2l927_1 {
        actions = {
            dashpipelinebmv2l927_1();
        }
        const default_action = dashpipelinebmv2l927_1();
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
    @hidden table tbl_dashpipelinebmv2l466 {
        actions = {
            dashpipelinebmv2l466();
        }
        const default_action = dashpipelinebmv2l466();
    }
    @hidden table tbl_dashpipelinebmv2l469 {
        actions = {
            dashpipelinebmv2l469();
        }
        const default_action = dashpipelinebmv2l469();
    }
    @hidden table tbl_dashpipelinebmv2l476 {
        actions = {
            dashpipelinebmv2l476();
        }
        const default_action = dashpipelinebmv2l476();
    }
    @hidden table tbl_dashpipelinebmv2l479 {
        actions = {
            dashpipelinebmv2l479();
        }
        const default_action = dashpipelinebmv2l479();
    }
    @hidden table tbl_dashpipelinebmv2l613 {
        actions = {
            dashpipelinebmv2l613();
        }
        const default_action = dashpipelinebmv2l613();
    }
    @hidden table tbl_vxlan_encap {
        actions = {
            vxlan_encap_1();
        }
        const default_action = vxlan_encap_1();
    }
    @hidden table tbl_vxlan_encap_0 {
        actions = {
            vxlan_encap_2();
        }
        const default_action = vxlan_encap_2();
    }
    @hidden table tbl_nvgre_encap {
        actions = {
            nvgre_encap_0();
        }
        const default_action = nvgre_encap_0();
    }
    @hidden table tbl_outbound_drop {
        actions = {
            outbound_drop_2();
        }
        const default_action = outbound_drop_2();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_dashpipelinebmv2l466_0 {
        actions = {
            dashpipelinebmv2l466_0();
        }
        const default_action = dashpipelinebmv2l466_0();
    }
    @hidden table tbl_dashpipelinebmv2l469_0 {
        actions = {
            dashpipelinebmv2l469_0();
        }
        const default_action = dashpipelinebmv2l469_0();
    }
    @hidden table tbl_dashpipelinebmv2l476_0 {
        actions = {
            dashpipelinebmv2l476_0();
        }
        const default_action = dashpipelinebmv2l476_0();
    }
    @hidden table tbl_dashpipelinebmv2l479_0 {
        actions = {
            dashpipelinebmv2l479_0();
        }
        const default_action = dashpipelinebmv2l479_0();
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
    @hidden table tbl_dashpipelinebmv2l947 {
        actions = {
            dashpipelinebmv2l947();
        }
        const default_action = dashpipelinebmv2l947();
    }
    @hidden table tbl_dashpipelinebmv2l949 {
        actions = {
            dashpipelinebmv2l949();
        }
        const default_action = dashpipelinebmv2l949();
    }
    @hidden table tbl_dashpipelinebmv2l952 {
        actions = {
            dashpipelinebmv2l952();
        }
        const default_action = dashpipelinebmv2l952();
    }
    @hidden table tbl_dashpipelinebmv2l957 {
        actions = {
            dashpipelinebmv2l957();
        }
        const default_action = dashpipelinebmv2l957();
    }
    @hidden table tbl_dashpipelinebmv2l959 {
        actions = {
            dashpipelinebmv2l959();
        }
        const default_action = dashpipelinebmv2l959();
    }
    @hidden table tbl_drop_action {
        actions = {
            drop_action();
        }
        const default_action = drop_action();
    }
    apply {
        tbl_dashpipelinebmv2l890.apply();
        if (vip_0.apply().hit) {
            tbl_dashpipelinebmv2l892.apply();
        }
        direction_lookup_0.apply();
        appliance_0.apply();
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
        tbl_dashpipelinebmv2l906.apply();
        if (hdr.ipv6.isValid()) {
            tbl_dashpipelinebmv2l911.apply();
        } else if (hdr.ipv4.isValid()) {
            tbl_dashpipelinebmv2l916.apply();
        }
        if (hdr.tcp.isValid()) {
            tbl_dashpipelinebmv2l921.apply();
        } else if (hdr.udp.isValid()) {
            tbl_dashpipelinebmv2l924.apply();
        }
        if (meta._direction1 == 16w1) {
            tbl_dashpipelinebmv2l927.apply();
        } else {
            tbl_dashpipelinebmv2l927_0.apply();
        }
        tbl_dashpipelinebmv2l927_1.apply();
        eni_ether_address_map_0.apply();
        eni_0.apply();
        if (meta._eni_data_admin_state20 == 1w0) {
            tbl_deny.apply();
        }
        acl_group_0.apply();
        src_tag_0.apply();
        dst_tag_0.apply();
        if (meta._direction1 == 16w1) {
            if (meta._conntrack_data_allow_out30) {
                ;
            } else {
                tbl_act.apply();
                if (meta._stage1_dash_acl_group_id33 != 16w0) {
                    switch (outbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
                        outbound_acl_permit_0: {
                            tbl_dashpipelinebmv2l466.apply();
                        }
                        outbound_acl_deny_0: {
                            tbl_dashpipelinebmv2l469.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id34 != 16w0) {
                    switch (outbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
                        outbound_acl_permit_1: {
                            tbl_dashpipelinebmv2l476.apply();
                        }
                        outbound_acl_deny_1: {
                            tbl_dashpipelinebmv2l479.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id35 != 16w0) {
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
            tbl_dashpipelinebmv2l613.apply();
            switch (outbound_outbound_routing_dash_outbound_routing.apply().action_run) {
                outbound_route_vnet_direct_0: 
                outbound_route_vnet_0: {
                    outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa.apply();
                    outbound_vnet_dash_vnet.apply();
                    tbl_vxlan_encap.apply();
                }
                outbound_route_service_tunnel_0: {
                    if (meta._encap_data_dash_encapsulation9 == 16w1) {
                        tbl_vxlan_encap_0.apply();
                    } else if (meta._encap_data_dash_encapsulation9 == 16w2) {
                        tbl_nvgre_encap.apply();
                    } else {
                        tbl_outbound_drop.apply();
                    }
                }
                default: {
                }
            }
        } else if (meta._direction1 == 16w2) {
            if (meta._conntrack_data_allow_in29) {
                ;
            } else {
                tbl_act_0.apply();
                if (meta._stage1_dash_acl_group_id33 != 16w0) {
                    switch (inbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
                        inbound_acl_permit_0: {
                            tbl_dashpipelinebmv2l466_0.apply();
                        }
                        inbound_acl_deny_0: {
                            tbl_dashpipelinebmv2l469_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id34 != 16w0) {
                    switch (inbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
                        inbound_acl_permit_1: {
                            tbl_dashpipelinebmv2l476_0.apply();
                        }
                        inbound_acl_deny_1: {
                            tbl_dashpipelinebmv2l479_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id35 != 16w0) {
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
            tbl_act_1.apply();
            tbl_vxlan_encap_1.apply();
        }
        if (meta._meter_policy_en38 == 1w1) {
            meter_policy_0.apply();
            meter_rule_0.apply();
        }
        if (meta._meter_policy_en38 == 1w1) {
            tbl_dashpipelinebmv2l947.apply();
        } else {
            tbl_dashpipelinebmv2l949.apply();
        }
        if (meta._meter_class44 == 16w0 || meta._mapping_meter_class_override39 == 1w1) {
            tbl_dashpipelinebmv2l952.apply();
        }
        meter_bucket_0.apply();
        if (meta._direction1 == 16w1) {
            tbl_dashpipelinebmv2l957.apply();
        } else if (meta._direction1 == 16w2) {
            tbl_dashpipelinebmv2l959.apply();
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
