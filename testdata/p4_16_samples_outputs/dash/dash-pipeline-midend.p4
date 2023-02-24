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
    bit<48>  _eni_addr9;
    bit<16>  _vnet_id10;
    bit<16>  _dst_vnet_id11;
    bit<16>  _eni_id12;
    bit<32>  _eni_data_cps13;
    bit<32>  _eni_data_pps14;
    bit<32>  _eni_data_flows15;
    bit<1>   _eni_data_admin_state16;
    bit<16>  _inbound_vm_id17;
    bit<8>   _appliance_id18;
    bit<1>   _is_overlay_ip_v619;
    bit<1>   _is_lkup_dst_ip_v620;
    bit<8>   _ip_protocol21;
    bit<128> _dst_ip_addr22;
    bit<128> _src_ip_addr23;
    bit<128> _lkup_dst_ip_addr24;
    bool     _conntrack_data_allow_in25;
    bool     _conntrack_data_allow_out26;
    bit<16>  _src_l4_port27;
    bit<16>  _dst_l4_port28;
    bit<16>  _stage1_dash_acl_group_id29;
    bit<16>  _stage2_dash_acl_group_id30;
    bit<16>  _stage3_dash_acl_group_id31;
    bit<16>  _stage4_dash_acl_group_id32;
    bit<16>  _stage5_dash_acl_group_id33;
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

control dash_verify_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control dash_compute_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control dash_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @name("dash_ingress.tmp_0") bit<48> tmp_0;
    @name("dash_ingress.inbound.tmp") bit<48> inbound_tmp;
    @name("dash_ingress.outbound.acl.hasReturned") bool outbound_acl_hasReturned;
    @name("dash_ingress.inbound.acl.hasReturned") bool inbound_acl_hasReturned;
    udp_t hdr_0_udp;
    vxlan_t hdr_0_vxlan;
    ethernet_t hdr_0_inner_ethernet;
    ipv4_t hdr_0_inner_ipv4;
    ipv6_t hdr_0_inner_ipv6;
    udp_t hdr_0_inner_udp;
    tcp_t hdr_0_inner_tcp;
    udp_t hdr_1_udp;
    vxlan_t hdr_1_vxlan;
    ethernet_t hdr_1_inner_ethernet;
    ipv4_t hdr_1_inner_ipv4;
    ipv6_t hdr_1_inner_ipv6;
    udp_t hdr_1_inner_udp;
    tcp_t hdr_1_inner_tcp;
    udp_t hdr_7_udp;
    vxlan_t hdr_7_vxlan;
    ethernet_t hdr_7_inner_ethernet;
    ipv4_t hdr_7_inner_ipv4;
    ipv6_t hdr_7_inner_ipv6;
    udp_t hdr_7_inner_udp;
    tcp_t hdr_7_inner_tcp;
    ethernet_t hdr_8_ethernet;
    ipv4_t hdr_8_ipv4;
    ipv6_t hdr_8_ipv6;
    udp_t hdr_8_udp;
    tcp_t hdr_8_tcp;
    vxlan_t hdr_8_vxlan;
    ethernet_t hdr_8_inner_ethernet;
    ipv4_t hdr_8_inner_ipv4;
    ipv6_t hdr_8_inner_ipv6;
    udp_t hdr_8_inner_udp;
    tcp_t hdr_8_inner_tcp;
    ethernet_t hdr_9_ethernet;
    ipv4_t hdr_9_ipv4;
    ipv6_t hdr_9_ipv6;
    udp_t hdr_9_udp;
    tcp_t hdr_9_tcp;
    vxlan_t hdr_9_vxlan;
    ethernet_t hdr_9_inner_ethernet;
    ipv4_t hdr_9_inner_ipv4;
    ipv6_t hdr_9_inner_ipv6;
    udp_t hdr_9_inner_udp;
    tcp_t hdr_9_inner_tcp;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name(".vxlan_decap") action vxlan_decap_1() {
        hdr_0_udp = hdr.udp;
        hdr_0_vxlan = hdr.vxlan;
        hdr_0_inner_ethernet = hdr.inner_ethernet;
        hdr_0_inner_ipv4 = hdr.inner_ipv4;
        hdr_0_inner_ipv6 = hdr.inner_ipv6;
        hdr_0_inner_udp = hdr.inner_udp;
        hdr_0_inner_tcp = hdr.inner_tcp;
        hdr_0_inner_ethernet.setInvalid();
        hdr_0_inner_ipv4.setInvalid();
        hdr_0_inner_ipv6.setInvalid();
        hdr_0_vxlan.setInvalid();
        hdr_0_udp.setInvalid();
        hdr_0_inner_tcp.setInvalid();
        hdr_0_udp = hdr.inner_udp;
        hdr_0_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_0_vxlan;
        hdr.inner_ethernet = hdr_0_inner_ethernet;
        hdr.inner_ipv4 = hdr_0_inner_ipv4;
        hdr.inner_ipv6 = hdr_0_inner_ipv6;
        hdr.inner_udp = hdr_0_inner_udp;
        hdr.inner_tcp = hdr_0_inner_tcp;
    }
    @name(".vxlan_decap") action vxlan_decap_2() {
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
    @name(".vxlan_decap") action vxlan_decap_3() {
        hdr_7_udp = hdr.udp;
        hdr_7_vxlan = hdr.vxlan;
        hdr_7_inner_ethernet = hdr.inner_ethernet;
        hdr_7_inner_ipv4 = hdr.inner_ipv4;
        hdr_7_inner_ipv6 = hdr.inner_ipv6;
        hdr_7_inner_udp = hdr.inner_udp;
        hdr_7_inner_tcp = hdr.inner_tcp;
        hdr_7_inner_ethernet.setInvalid();
        hdr_7_inner_ipv4.setInvalid();
        hdr_7_inner_ipv6.setInvalid();
        hdr_7_vxlan.setInvalid();
        hdr_7_udp.setInvalid();
        hdr_7_inner_tcp.setInvalid();
        hdr_7_udp = hdr.inner_udp;
        hdr_7_inner_udp.setInvalid();
        hdr.ethernet = hdr.inner_ethernet;
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.ipv6 = hdr.inner_ipv6;
        hdr.udp = hdr.inner_udp;
        hdr.tcp = hdr.inner_tcp;
        hdr.vxlan = hdr_7_vxlan;
        hdr.inner_ethernet = hdr_7_inner_ethernet;
        hdr.inner_ipv4 = hdr_7_inner_ipv4;
        hdr.inner_ipv6 = hdr_7_inner_ipv6;
        hdr.inner_udp = hdr_7_inner_udp;
        hdr.inner_tcp = hdr_7_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_1() {
        hdr_8_ethernet = hdr.ethernet;
        hdr_8_ipv4 = hdr.ipv4;
        hdr_8_ipv6 = hdr.ipv6;
        hdr_8_udp = hdr.udp;
        hdr_8_tcp = hdr.tcp;
        hdr_8_vxlan = hdr.vxlan;
        hdr_8_inner_ethernet = hdr.inner_ethernet;
        hdr_8_inner_ipv4 = hdr.inner_ipv4;
        hdr_8_inner_ipv6 = hdr.inner_ipv6;
        hdr_8_inner_udp = hdr.inner_udp;
        hdr_8_inner_tcp = hdr.inner_tcp;
        hdr_8_inner_ethernet = hdr.ethernet;
        hdr_8_inner_ethernet.dst_addr = meta._encap_data_overlay_dmac8;
        hdr_8_ethernet.setInvalid();
        hdr_8_inner_ipv4 = hdr.ipv4;
        hdr_8_ipv4.setInvalid();
        hdr_8_inner_ipv6 = hdr.ipv6;
        hdr_8_ipv6.setInvalid();
        hdr_8_inner_tcp = hdr.tcp;
        hdr_8_tcp.setInvalid();
        hdr_8_inner_udp = hdr.udp;
        hdr_8_udp.setInvalid();
        hdr_8_ethernet.setValid();
        hdr_8_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_8_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_8_ethernet.ether_type = 16w0x800;
        hdr_8_ipv4.setValid();
        hdr_8_ipv4.version = 4w4;
        hdr_8_ipv4.ihl = 4w5;
        hdr_8_ipv4.diffserv = 8w0;
        hdr_8_ipv4.total_len = hdr_8_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_8_inner_ipv4.isValid() + hdr_8_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_8_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_8_inner_ipv6.isValid() + 16w50;
        hdr_8_ipv4.identification = 16w1;
        hdr_8_ipv4.flags = 3w0;
        hdr_8_ipv4.frag_offset = 13w0;
        hdr_8_ipv4.ttl = 8w64;
        hdr_8_ipv4.protocol = 8w17;
        hdr_8_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_8_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_8_ipv4.hdr_checksum = 16w0;
        hdr_8_udp.setValid();
        hdr_8_udp.src_port = 16w0;
        hdr_8_udp.dst_port = 16w4789;
        hdr_8_udp.length = hdr_8_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_8_inner_ipv4.isValid() + hdr_8_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_8_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_8_inner_ipv6.isValid() + 16w30;
        hdr_8_udp.checksum = 16w0;
        hdr_8_vxlan.setValid();
        hdr_8_vxlan.reserved = 24w0;
        hdr_8_vxlan.reserved_2 = 8w0;
        hdr_8_vxlan.flags = 8w0;
        hdr_8_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_8_ethernet;
        hdr.ipv4 = hdr_8_ipv4;
        hdr.ipv6 = hdr_8_ipv6;
        hdr.udp = hdr_8_udp;
        hdr.tcp = hdr_8_tcp;
        hdr.vxlan = hdr_8_vxlan;
        hdr.inner_ethernet = hdr_8_inner_ethernet;
        hdr.inner_ipv4 = hdr_8_inner_ipv4;
        hdr.inner_ipv6 = hdr_8_inner_ipv6;
        hdr.inner_udp = hdr_8_inner_udp;
        hdr.inner_tcp = hdr_8_inner_tcp;
    }
    @name(".vxlan_encap") action vxlan_encap_2() {
        hdr_9_ethernet = hdr.ethernet;
        hdr_9_ipv4 = hdr.ipv4;
        hdr_9_ipv6 = hdr.ipv6;
        hdr_9_udp = hdr.udp;
        hdr_9_tcp = hdr.tcp;
        hdr_9_vxlan = hdr.vxlan;
        hdr_9_inner_ethernet = hdr.inner_ethernet;
        hdr_9_inner_ipv4 = hdr.inner_ipv4;
        hdr_9_inner_ipv6 = hdr.inner_ipv6;
        hdr_9_inner_udp = hdr.inner_udp;
        hdr_9_inner_tcp = hdr.inner_tcp;
        hdr_9_inner_ethernet = hdr.ethernet;
        hdr_9_inner_ethernet.dst_addr = inbound_tmp;
        hdr_9_ethernet.setInvalid();
        hdr_9_inner_ipv4 = hdr.ipv4;
        hdr_9_ipv4.setInvalid();
        hdr_9_inner_ipv6 = hdr.ipv6;
        hdr_9_ipv6.setInvalid();
        hdr_9_inner_tcp = hdr.tcp;
        hdr_9_tcp.setInvalid();
        hdr_9_inner_udp = hdr.udp;
        hdr_9_udp.setInvalid();
        hdr_9_ethernet.setValid();
        hdr_9_ethernet.dst_addr = meta._encap_data_underlay_dmac7;
        hdr_9_ethernet.src_addr = meta._encap_data_underlay_smac6;
        hdr_9_ethernet.ether_type = 16w0x800;
        hdr_9_ipv4.setValid();
        hdr_9_ipv4.version = 4w4;
        hdr_9_ipv4.ihl = 4w5;
        hdr_9_ipv4.diffserv = 8w0;
        hdr_9_ipv4.total_len = hdr_9_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_9_inner_ipv4.isValid() + hdr_9_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_9_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_9_inner_ipv6.isValid() + 16w50;
        hdr_9_ipv4.identification = 16w1;
        hdr_9_ipv4.flags = 3w0;
        hdr_9_ipv4.frag_offset = 13w0;
        hdr_9_ipv4.ttl = 8w64;
        hdr_9_ipv4.protocol = 8w17;
        hdr_9_ipv4.dst_addr = meta._encap_data_underlay_dip5;
        hdr_9_ipv4.src_addr = meta._encap_data_underlay_sip4;
        hdr_9_ipv4.hdr_checksum = 16w0;
        hdr_9_udp.setValid();
        hdr_9_udp.src_port = 16w0;
        hdr_9_udp.dst_port = 16w4789;
        hdr_9_udp.length = hdr_9_inner_ipv4.total_len * (bit<16>)(bit<1>)hdr_9_inner_ipv4.isValid() + hdr_9_inner_ipv6.payload_length * (bit<16>)(bit<1>)hdr_9_inner_ipv6.isValid() + 16w40 * (bit<16>)(bit<1>)hdr_9_inner_ipv6.isValid() + 16w30;
        hdr_9_udp.checksum = 16w0;
        hdr_9_vxlan.setValid();
        hdr_9_vxlan.reserved = 24w0;
        hdr_9_vxlan.reserved_2 = 8w0;
        hdr_9_vxlan.flags = 8w0;
        hdr_9_vxlan.vni = meta._encap_data_vni2;
        hdr.ethernet = hdr_9_ethernet;
        hdr.ipv4 = hdr_9_ipv4;
        hdr.ipv6 = hdr_9_ipv6;
        hdr.udp = hdr_9_udp;
        hdr.tcp = hdr_9_tcp;
        hdr.vxlan = hdr_9_vxlan;
        hdr.inner_ethernet = hdr_9_inner_ethernet;
        hdr.inner_ipv4 = hdr_9_inner_ipv4;
        hdr.inner_ipv6 = hdr_9_inner_ipv6;
        hdr.inner_udp = hdr_9_inner_udp;
        hdr.inner_tcp = hdr_9_inner_tcp;
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
            meta._stage1_dash_acl_group_id29: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
            meta._stage2_dash_acl_group_id30: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
            meta._stage3_dash_acl_group_id31: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
    @name("dash_ingress.outbound.route_vnet") action outbound_route_vnet_0(@name("dst_vnet_id") bit<16> dst_vnet_id_2) {
        meta._dst_vnet_id11 = dst_vnet_id_2;
    }
    @name("dash_ingress.outbound.route_vnet_direct") action outbound_route_vnet_direct_0(@name("dst_vnet_id") bit<16> dst_vnet_id_3, @name("is_overlay_ip_v4_or_v6") bit<1> is_overlay_ip_v4_or_v6, @name("overlay_ip") bit<128> overlay_ip) {
        meta._dst_vnet_id11 = dst_vnet_id_3;
        meta._lkup_dst_ip_addr24 = overlay_ip;
        meta._is_lkup_dst_ip_v620 = is_overlay_ip_v4_or_v6;
    }
    @name("dash_ingress.outbound.route_direct") action outbound_route_direct_0() {
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_0() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.drop") action outbound_drop_1() {
        meta._dropped0 = true;
    }
    @name("dash_ingress.outbound.routing_counter") direct_counter(CounterType.packets_and_bytes) outbound_routing_counter;
    @name("dash_ingress.outbound.outbound_routing|dash_outbound_routing") table outbound_outbound_routing_dash_outbound_routing {
        key = {
            meta._eni_id12          : exact @name("meta.eni_id:eni_id");
            meta._is_overlay_ip_v619: exact @name("meta.is_overlay_ip_v6:is_destination_v4_or_v6");
            meta._dst_ip_addr22     : lpm @name("meta.dst_ip_addr:destination");
        }
        actions = {
            outbound_route_vnet_0();
            outbound_route_vnet_direct_0();
            outbound_route_direct_0();
            outbound_drop_0();
        }
        const default_action = outbound_drop_0();
        counters = outbound_routing_counter;
    }
    @name("dash_ingress.outbound.set_tunnel_mapping") action outbound_set_tunnel_mapping_0(@name("underlay_dip") bit<32> underlay_dip_4, @name("overlay_dmac") bit<48> overlay_dmac_4, @name("use_dst_vnet_vni") bit<1> use_dst_vnet_vni) {
        meta._vnet_id10 = (use_dst_vnet_vni == 1w1 ? meta._dst_vnet_id11 : meta._vnet_id10);
        meta._encap_data_overlay_dmac8 = overlay_dmac_4;
        meta._encap_data_underlay_dip5 = underlay_dip_4;
    }
    @name("dash_ingress.outbound.ca_to_pa_counter") direct_counter(CounterType.packets_and_bytes) outbound_ca_to_pa_counter;
    @name("dash_ingress.outbound.outbound_ca_to_pa|dash_outbound_ca_to_pa") table outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa {
        key = {
            meta._dst_vnet_id11      : exact @name("meta.dst_vnet_id:dst_vnet_id");
            meta._is_lkup_dst_ip_v620: exact @name("meta.is_lkup_dst_ip_v6:is_dip_v4_or_v6");
            meta._lkup_dst_ip_addr24 : exact @name("meta.lkup_dst_ip_addr:dip");
        }
        actions = {
            outbound_set_tunnel_mapping_0();
            @defaultonly outbound_drop_1();
        }
        const default_action = outbound_drop_1();
        counters = outbound_ca_to_pa_counter;
    }
    @name("dash_ingress.outbound.set_vnet_attrs") action outbound_set_vnet_attrs_0(@name("vni") bit<24> vni_4) {
        meta._encap_data_vni2 = vni_4;
    }
    @name("dash_ingress.outbound.vnet|dash_vnet") table outbound_vnet_dash_vnet {
        key = {
            meta._vnet_id10: exact @name("meta.vnet_id:vnet_id");
        }
        actions = {
            outbound_set_vnet_attrs_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
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
            meta._stage1_dash_acl_group_id29: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
            meta._stage2_dash_acl_group_id30: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
            meta._stage3_dash_acl_group_id31: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta._dst_ip_addr22             : optional @name("meta.dst_ip_addr:dip");
            meta._src_ip_addr23             : optional @name("meta.src_ip_addr:sip");
            meta._ip_protocol21             : optional @name("meta.ip_protocol:protocol");
            meta._src_l4_port27             : optional @name("meta.src_l4_port:src_port");
            meta._dst_l4_port28             : optional @name("meta.dst_l4_port:dst_port");
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
            meta._appliance_id18: ternary @name("meta.appliance_id:appliance_id");
        }
        actions = {
            set_appliance();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("dash_ingress.set_eni_attrs") action set_eni_attrs(@name("cps") bit<32> cps_1, @name("pps") bit<32> pps_1, @name("flows") bit<32> flows_1, @name("admin_state") bit<1> admin_state_1, @name("vm_underlay_dip") bit<32> vm_underlay_dip, @name("vm_vni") bit<24> vm_vni, @name("vnet_id") bit<16> vnet_id_1, @name("inbound_v4_stage1_dash_acl_group_id") bit<16> inbound_v4_stage1_dash_acl_group_id, @name("inbound_v4_stage2_dash_acl_group_id") bit<16> inbound_v4_stage2_dash_acl_group_id, @name("inbound_v4_stage3_dash_acl_group_id") bit<16> inbound_v4_stage3_dash_acl_group_id, @name("inbound_v4_stage4_dash_acl_group_id") bit<16> inbound_v4_stage4_dash_acl_group_id, @name("inbound_v4_stage5_dash_acl_group_id") bit<16> inbound_v4_stage5_dash_acl_group_id, @name("inbound_v6_stage1_dash_acl_group_id") bit<16> inbound_v6_stage1_dash_acl_group_id, @name("inbound_v6_stage2_dash_acl_group_id") bit<16> inbound_v6_stage2_dash_acl_group_id, @name("inbound_v6_stage3_dash_acl_group_id") bit<16> inbound_v6_stage3_dash_acl_group_id, @name("inbound_v6_stage4_dash_acl_group_id") bit<16> inbound_v6_stage4_dash_acl_group_id, @name("inbound_v6_stage5_dash_acl_group_id") bit<16> inbound_v6_stage5_dash_acl_group_id, @name("outbound_v4_stage1_dash_acl_group_id") bit<16> outbound_v4_stage1_dash_acl_group_id, @name("outbound_v4_stage2_dash_acl_group_id") bit<16> outbound_v4_stage2_dash_acl_group_id, @name("outbound_v4_stage3_dash_acl_group_id") bit<16> outbound_v4_stage3_dash_acl_group_id, @name("outbound_v4_stage4_dash_acl_group_id") bit<16> outbound_v4_stage4_dash_acl_group_id, @name("outbound_v4_stage5_dash_acl_group_id") bit<16> outbound_v4_stage5_dash_acl_group_id, @name("outbound_v6_stage1_dash_acl_group_id") bit<16> outbound_v6_stage1_dash_acl_group_id, @name("outbound_v6_stage2_dash_acl_group_id") bit<16> outbound_v6_stage2_dash_acl_group_id, @name("outbound_v6_stage3_dash_acl_group_id") bit<16> outbound_v6_stage3_dash_acl_group_id, @name("outbound_v6_stage4_dash_acl_group_id") bit<16> outbound_v6_stage4_dash_acl_group_id, @name("outbound_v6_stage5_dash_acl_group_id") bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta._eni_data_cps13 = cps_1;
        meta._eni_data_pps14 = pps_1;
        meta._eni_data_flows15 = flows_1;
        meta._eni_data_admin_state16 = admin_state_1;
        meta._encap_data_underlay_dip5 = vm_underlay_dip;
        meta._encap_data_vni2 = vm_vni;
        meta._vnet_id10 = vnet_id_1;
        meta._stage1_dash_acl_group_id29 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage1_dash_acl_group_id : meta._stage1_dash_acl_group_id29) : meta._stage1_dash_acl_group_id29);
        meta._stage2_dash_acl_group_id30 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage2_dash_acl_group_id : meta._stage2_dash_acl_group_id30) : meta._stage2_dash_acl_group_id30);
        meta._stage3_dash_acl_group_id31 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage3_dash_acl_group_id : meta._stage3_dash_acl_group_id31) : meta._stage3_dash_acl_group_id31);
        meta._stage4_dash_acl_group_id32 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage4_dash_acl_group_id : meta._stage4_dash_acl_group_id32) : meta._stage4_dash_acl_group_id32);
        meta._stage5_dash_acl_group_id33 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage5_dash_acl_group_id : meta._stage5_dash_acl_group_id33) : meta._stage5_dash_acl_group_id33);
        meta._stage1_dash_acl_group_id29 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage1_dash_acl_group_id : inbound_v6_stage1_dash_acl_group_id) : meta._stage1_dash_acl_group_id29);
        meta._stage2_dash_acl_group_id30 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage2_dash_acl_group_id : inbound_v6_stage2_dash_acl_group_id) : meta._stage2_dash_acl_group_id30);
        meta._stage3_dash_acl_group_id31 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage3_dash_acl_group_id : inbound_v6_stage3_dash_acl_group_id) : meta._stage3_dash_acl_group_id31);
        meta._stage4_dash_acl_group_id32 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage4_dash_acl_group_id : inbound_v6_stage4_dash_acl_group_id) : meta._stage4_dash_acl_group_id32);
        meta._stage5_dash_acl_group_id33 = (meta._is_overlay_ip_v619 == 1w1 ? (meta._direction1 == 16w1 ? outbound_v6_stage5_dash_acl_group_id : inbound_v6_stage5_dash_acl_group_id) : meta._stage5_dash_acl_group_id33);
        meta._stage1_dash_acl_group_id29 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage1_dash_acl_group_id29 : (meta._direction1 == 16w1 ? outbound_v4_stage1_dash_acl_group_id : meta._stage1_dash_acl_group_id29));
        meta._stage2_dash_acl_group_id30 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage2_dash_acl_group_id30 : (meta._direction1 == 16w1 ? outbound_v4_stage2_dash_acl_group_id : meta._stage2_dash_acl_group_id30));
        meta._stage3_dash_acl_group_id31 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage3_dash_acl_group_id31 : (meta._direction1 == 16w1 ? outbound_v4_stage3_dash_acl_group_id : meta._stage3_dash_acl_group_id31));
        meta._stage4_dash_acl_group_id32 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage4_dash_acl_group_id32 : (meta._direction1 == 16w1 ? outbound_v4_stage4_dash_acl_group_id : meta._stage4_dash_acl_group_id32));
        meta._stage5_dash_acl_group_id33 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage5_dash_acl_group_id33 : (meta._direction1 == 16w1 ? outbound_v4_stage5_dash_acl_group_id : meta._stage5_dash_acl_group_id33));
        meta._stage1_dash_acl_group_id29 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage1_dash_acl_group_id29 : (meta._direction1 == 16w1 ? outbound_v4_stage1_dash_acl_group_id : inbound_v4_stage1_dash_acl_group_id));
        meta._stage2_dash_acl_group_id30 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage2_dash_acl_group_id30 : (meta._direction1 == 16w1 ? outbound_v4_stage2_dash_acl_group_id : inbound_v4_stage2_dash_acl_group_id));
        meta._stage3_dash_acl_group_id31 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage3_dash_acl_group_id31 : (meta._direction1 == 16w1 ? outbound_v4_stage3_dash_acl_group_id : inbound_v4_stage3_dash_acl_group_id));
        meta._stage4_dash_acl_group_id32 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage4_dash_acl_group_id32 : (meta._direction1 == 16w1 ? outbound_v4_stage4_dash_acl_group_id : inbound_v4_stage4_dash_acl_group_id));
        meta._stage5_dash_acl_group_id33 = (meta._is_overlay_ip_v619 == 1w1 ? meta._stage5_dash_acl_group_id33 : (meta._direction1 == 16w1 ? outbound_v4_stage5_dash_acl_group_id : inbound_v4_stage5_dash_acl_group_id));
    }
    @name("dash_ingress.eni|dash_eni") table eni_0 {
        key = {
            meta._eni_id12: exact @name("meta.eni_id:eni_id");
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
            meta._eni_id12  : exact @name("meta.eni_id:eni_id");
            meta._direction1: exact @name("meta.direction:direction");
            meta._dropped0  : exact @name("meta.dropped:dropped");
        }
        actions = {
            NoAction_3();
        }
        counters = eni_counter_0;
        default_action = NoAction_3();
    }
    @name("dash_ingress.permit") action permit() {
    }
    @name("dash_ingress.vxlan_decap_pa_validate") action vxlan_decap_pa_validate(@name("src_vnet_id") bit<16> src_vnet_id) {
        meta._vnet_id10 = src_vnet_id;
    }
    @name("dash_ingress.pa_validation|dash_pa_validation") table pa_validation_0 {
        key = {
            meta._vnet_id10  : exact @name("meta.vnet_id:vnet_id");
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
            meta._eni_id12   : exact @name("meta.eni_id:eni_id");
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
    @name("dash_ingress.set_eni") action set_eni(@name("eni_id") bit<16> eni_id_1) {
        meta._eni_id12 = eni_id_1;
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
    @name("dash_ingress.set_acl_group_attrs") action set_acl_group_attrs(@name("ip_addr_family") bit<32> ip_addr_family) {
        meta._dropped0 = (ip_addr_family == 32w0 ? (meta._is_overlay_ip_v619 == 1w1 ? true : meta._dropped0) : meta._dropped0);
        meta._dropped0 = (ip_addr_family == 32w0 ? meta._dropped0 : (meta._is_overlay_ip_v619 == 1w0 ? true : meta._dropped0));
    }
    @name("dash_ingress.dash_acl_group|dash_acl") table acl_group_0 {
        key = {
            meta._stage1_dash_acl_group_id29: exact @name("meta.stage1_dash_acl_group_id:dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
            @defaultonly NoAction_4();
        }
        default_action = NoAction_4();
    }
    @hidden action dashpipeline721() {
        meta._encap_data_underlay_sip4 = hdr.ipv4.dst_addr;
    }
    @hidden action dashpipeline719() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @hidden action dashpipeline740() {
        meta._ip_protocol21 = hdr.ipv6.next_header;
        meta._src_ip_addr23 = hdr.ipv6.src_addr;
        meta._dst_ip_addr22 = hdr.ipv6.dst_addr;
        meta._is_overlay_ip_v619 = 1w1;
    }
    @hidden action dashpipeline745() {
        meta._ip_protocol21 = hdr.ipv4.protocol;
        meta._src_ip_addr23 = (bit<128>)hdr.ipv4.src_addr;
        meta._dst_ip_addr22 = (bit<128>)hdr.ipv4.dst_addr;
    }
    @hidden action dashpipeline735() {
        meta._is_overlay_ip_v619 = 1w0;
        meta._ip_protocol21 = 8w0;
        meta._dst_ip_addr22 = 128w0;
        meta._src_ip_addr23 = 128w0;
    }
    @hidden action dashpipeline750() {
        meta._src_l4_port27 = hdr.tcp.src_port;
        meta._dst_l4_port28 = hdr.tcp.dst_port;
    }
    @hidden action dashpipeline753() {
        meta._src_l4_port27 = hdr.udp.src_port;
        meta._dst_l4_port28 = hdr.udp.dst_port;
    }
    @hidden action dashpipeline756() {
        tmp_0 = hdr.ethernet.src_addr;
    }
    @hidden action dashpipeline756_0() {
        tmp_0 = hdr.ethernet.dst_addr;
    }
    @hidden action dashpipeline756_1() {
        meta._eni_addr9 = tmp_0;
    }
    @hidden action dashpipeline390() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipeline393() {
        outbound_acl_hasReturned = true;
    }
    @hidden action act() {
        outbound_acl_hasReturned = false;
    }
    @hidden action dashpipeline400() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipeline403() {
        outbound_acl_hasReturned = true;
    }
    @hidden action dashpipeline486() {
        meta._lkup_dst_ip_addr24 = meta._dst_ip_addr22;
        meta._is_lkup_dst_ip_v620 = meta._is_overlay_ip_v619;
    }
    @hidden action dashpipeline390_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipeline393_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_0() {
        inbound_acl_hasReturned = false;
    }
    @hidden action dashpipeline400_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action dashpipeline403_0() {
        inbound_acl_hasReturned = true;
    }
    @hidden action act_1() {
        inbound_tmp = hdr.ethernet.dst_addr;
    }
    @hidden table tbl_dashpipeline719 {
        actions = {
            dashpipeline719();
        }
        const default_action = dashpipeline719();
    }
    @hidden table tbl_dashpipeline721 {
        actions = {
            dashpipeline721();
        }
        const default_action = dashpipeline721();
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
    @hidden table tbl_dashpipeline735 {
        actions = {
            dashpipeline735();
        }
        const default_action = dashpipeline735();
    }
    @hidden table tbl_dashpipeline740 {
        actions = {
            dashpipeline740();
        }
        const default_action = dashpipeline740();
    }
    @hidden table tbl_dashpipeline745 {
        actions = {
            dashpipeline745();
        }
        const default_action = dashpipeline745();
    }
    @hidden table tbl_dashpipeline750 {
        actions = {
            dashpipeline750();
        }
        const default_action = dashpipeline750();
    }
    @hidden table tbl_dashpipeline753 {
        actions = {
            dashpipeline753();
        }
        const default_action = dashpipeline753();
    }
    @hidden table tbl_dashpipeline756 {
        actions = {
            dashpipeline756();
        }
        const default_action = dashpipeline756();
    }
    @hidden table tbl_dashpipeline756_0 {
        actions = {
            dashpipeline756_0();
        }
        const default_action = dashpipeline756_0();
    }
    @hidden table tbl_dashpipeline756_1 {
        actions = {
            dashpipeline756_1();
        }
        const default_action = dashpipeline756_1();
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
    @hidden table tbl_dashpipeline390 {
        actions = {
            dashpipeline390();
        }
        const default_action = dashpipeline390();
    }
    @hidden table tbl_dashpipeline393 {
        actions = {
            dashpipeline393();
        }
        const default_action = dashpipeline393();
    }
    @hidden table tbl_dashpipeline400 {
        actions = {
            dashpipeline400();
        }
        const default_action = dashpipeline400();
    }
    @hidden table tbl_dashpipeline403 {
        actions = {
            dashpipeline403();
        }
        const default_action = dashpipeline403();
    }
    @hidden table tbl_dashpipeline486 {
        actions = {
            dashpipeline486();
        }
        const default_action = dashpipeline486();
    }
    @hidden table tbl_vxlan_encap {
        actions = {
            vxlan_encap_1();
        }
        const default_action = vxlan_encap_1();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_dashpipeline390_0 {
        actions = {
            dashpipeline390_0();
        }
        const default_action = dashpipeline390_0();
    }
    @hidden table tbl_dashpipeline393_0 {
        actions = {
            dashpipeline393_0();
        }
        const default_action = dashpipeline393_0();
    }
    @hidden table tbl_dashpipeline400_0 {
        actions = {
            dashpipeline400_0();
        }
        const default_action = dashpipeline400_0();
    }
    @hidden table tbl_dashpipeline403_0 {
        actions = {
            dashpipeline403_0();
        }
        const default_action = dashpipeline403_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_vxlan_encap_0 {
        actions = {
            vxlan_encap_2();
        }
        const default_action = vxlan_encap_2();
    }
    @hidden table tbl_drop_action {
        actions = {
            drop_action();
        }
        const default_action = drop_action();
    }
    apply {
        tbl_dashpipeline719.apply();
        if (vip_0.apply().hit) {
            tbl_dashpipeline721.apply();
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
        tbl_dashpipeline735.apply();
        if (hdr.ipv6.isValid()) {
            tbl_dashpipeline740.apply();
        } else if (hdr.ipv4.isValid()) {
            tbl_dashpipeline745.apply();
        }
        if (hdr.tcp.isValid()) {
            tbl_dashpipeline750.apply();
        } else if (hdr.udp.isValid()) {
            tbl_dashpipeline753.apply();
        }
        if (meta._direction1 == 16w1) {
            tbl_dashpipeline756.apply();
        } else {
            tbl_dashpipeline756_0.apply();
        }
        tbl_dashpipeline756_1.apply();
        eni_ether_address_map_0.apply();
        eni_0.apply();
        if (meta._eni_data_admin_state16 == 1w0) {
            tbl_deny.apply();
        }
        acl_group_0.apply();
        if (meta._direction1 == 16w1) {
            if (meta._conntrack_data_allow_out26) {
                ;
            } else {
                tbl_act.apply();
                if (meta._stage1_dash_acl_group_id29 != 16w0) {
                    switch (outbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
                        outbound_acl_permit_0: {
                            tbl_dashpipeline390.apply();
                        }
                        outbound_acl_deny_0: {
                            tbl_dashpipeline393.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id30 != 16w0) {
                    switch (outbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
                        outbound_acl_permit_1: {
                            tbl_dashpipeline400.apply();
                        }
                        outbound_acl_deny_1: {
                            tbl_dashpipeline403.apply();
                        }
                        default: {
                        }
                    }
                }
                if (outbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id31 != 16w0) {
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
            tbl_dashpipeline486.apply();
            switch (outbound_outbound_routing_dash_outbound_routing.apply().action_run) {
                outbound_route_vnet_direct_0: 
                outbound_route_vnet_0: {
                    outbound_outbound_ca_to_pa_dash_outbound_ca_to_pa.apply();
                    outbound_vnet_dash_vnet.apply();
                    tbl_vxlan_encap.apply();
                }
                default: {
                }
            }
        } else if (meta._direction1 == 16w2) {
            if (meta._conntrack_data_allow_in25) {
                ;
            } else {
                tbl_act_0.apply();
                if (meta._stage1_dash_acl_group_id29 != 16w0) {
                    switch (inbound_acl_stage1_dash_acl_rule_dash_acl.apply().action_run) {
                        inbound_acl_permit_0: {
                            tbl_dashpipeline390_0.apply();
                        }
                        inbound_acl_deny_0: {
                            tbl_dashpipeline393_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage2_dash_acl_group_id30 != 16w0) {
                    switch (inbound_acl_stage2_dash_acl_rule_dash_acl.apply().action_run) {
                        inbound_acl_permit_1: {
                            tbl_dashpipeline400_0.apply();
                        }
                        inbound_acl_deny_1: {
                            tbl_dashpipeline403_0.apply();
                        }
                        default: {
                        }
                    }
                }
                if (inbound_acl_hasReturned) {
                    ;
                } else if (meta._stage3_dash_acl_group_id31 != 16w0) {
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
            tbl_vxlan_encap_0.apply();
        }
        eni_meter_0.apply();
        if (meta._dropped0) {
            tbl_drop_action.apply();
        }
    }
}

control dash_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(dash_parser(), dash_verify_checksum(), dash_ingress(), dash_egress(), dash_compute_checksum(), dash_deparser()) main;
