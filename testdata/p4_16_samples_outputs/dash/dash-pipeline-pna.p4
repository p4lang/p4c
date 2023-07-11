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
    INVALID = 0,
    VXLAN = 1,
    NVGRE = 2
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
    INVALID = 0,
    OUTBOUND = 1,
    INBOUND = 2
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
        packet.extract(hd.ethernet);
        transition select(hd.ethernet.ether_type) {
            0x800: parse_ipv4;
            0x86dd: parse_ipv6;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hd.ipv4);
        verify(hd.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.ipv4.ihl >= 5, error.InvalidIPv4Header);
        transition select(hd.ipv4.ihl) {
            5: dispatch_on_protocol;
            default: parse_ipv4options;
        }
    }
    state parse_ipv4options {
        packet.extract(hd.ipv4options, (bit<32>)(((bit<16>)hd.ipv4.ihl - 5) * 32));
        transition dispatch_on_protocol;
    }
    state dispatch_on_protocol {
        transition select(hd.ipv4.protocol) {
            17: parse_udp;
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract(hd.ipv6);
        transition select(hd.ipv6.next_header) {
            17: parse_udp;
            6: parse_tcp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hd.udp);
        transition select(hd.udp.dst_port) {
            4789: parse_vxlan;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract(hd.tcp);
        transition accept;
    }
    state parse_vxlan {
        packet.extract(hd.vxlan);
        transition parse_inner_ethernet;
    }
    state parse_inner_ethernet {
        packet.extract(hd.inner_ethernet);
        transition select(hd.inner_ethernet.ether_type) {
            0x800: parse_inner_ipv4;
            0x86dd: parse_inner_ipv6;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract(hd.inner_ipv4);
        verify(hd.inner_ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hd.inner_ipv4.ihl == 4w5, error.IPv4OptionsNotSupported);
        transition select(hd.inner_ipv4.protocol) {
            17: parse_inner_udp;
            6: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_ipv6 {
        packet.extract(hd.inner_ipv6);
        transition select(hd.inner_ipv6.next_header) {
            17: parse_inner_udp;
            6: parse_inner_tcp;
            default: accept;
        }
    }
    state parse_inner_tcp {
        packet.extract(hd.inner_tcp);
        transition accept;
    }
    state parse_inner_udp {
        packet.extract(hd.inner_udp);
        transition accept;
    }
}

control dash_deparser(packet_out packet, in headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.ipv4options);
        packet.emit(hdr.ipv6);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        packet.emit(hdr.vxlan);
        packet.emit(hdr.nvgre);
        packet.emit(hdr.inner_ethernet);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_ipv6);
        packet.emit(hdr.inner_tcp);
        packet.emit(hdr.inner_udp);
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
    hdr.ethernet.ether_type = 0x800;
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4;
    hdr.ipv4.ihl = 5;
    hdr.ipv4.diffserv = 0;
    bit<16> inner_ip_len = 0;
    if (hdr.inner_ipv4.isValid()) {
        inner_ip_len = inner_ip_len + hdr.inner_ipv4.total_len;
    }
    if (hdr.inner_ipv6.isValid()) {
        inner_ip_len = inner_ip_len + IPV6_HDR_SIZE + hdr.inner_ipv6.payload_length;
    }
    hdr.ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + VXLAN_HDR_SIZE + inner_ip_len;
    hdr.ipv4.identification = 1;
    hdr.ipv4.flags = 0;
    hdr.ipv4.frag_offset = 0;
    hdr.ipv4.ttl = 64;
    hdr.ipv4.protocol = 17;
    hdr.ipv4.dst_addr = underlay_dip;
    hdr.ipv4.src_addr = underlay_sip;
    hdr.ipv4.hdr_checksum = 0;
    hdr.udp.setValid();
    hdr.udp.src_port = 0;
    hdr.udp.dst_port = 4789;
    hdr.udp.length = UDP_HDR_SIZE + VXLAN_HDR_SIZE + ETHER_HDR_SIZE + inner_ip_len;
    hdr.udp.checksum = 0;
    hdr.vxlan.setValid();
    hdr.vxlan.reserved = 0;
    hdr.vxlan.reserved_2 = 0;
    hdr.vxlan.flags = 0;
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
    hdr.ethernet.ether_type = 0x800;
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4;
    hdr.ipv4.ihl = 5;
    hdr.ipv4.diffserv = 0;
    bit<16> inner_ip_len = 0;
    if (hdr.inner_ipv4.isValid()) {
        inner_ip_len = inner_ip_len + hdr.inner_ipv4.total_len;
    }
    if (hdr.inner_ipv6.isValid()) {
        inner_ip_len = inner_ip_len + IPV6_HDR_SIZE + hdr.inner_ipv6.payload_length;
    }
    hdr.ipv4.total_len = ETHER_HDR_SIZE + IPV4_HDR_SIZE + UDP_HDR_SIZE + NVGRE_HDR_SIZE + inner_ip_len;
    hdr.ipv4.identification = 1;
    hdr.ipv4.flags = 0;
    hdr.ipv4.frag_offset = 0;
    hdr.ipv4.ttl = 64;
    hdr.ipv4.protocol = 0x2f;
    hdr.ipv4.dst_addr = underlay_dip;
    hdr.ipv4.src_addr = underlay_sip;
    hdr.ipv4.hdr_checksum = 0;
    hdr.nvgre.setValid();
    hdr.nvgre.flags = 4;
    hdr.nvgre.reserved = 0;
    hdr.nvgre.version = 0;
    hdr.nvgre.protocol_type = 0x6558;
    hdr.nvgre.vsid = vsid;
    hdr.nvgre.flow_id = 0;
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
    @name("stage1:dash_acl_rule|dash_acl") table stage1 {
        key = {
            meta.stage1_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            permit;
            permit_and_continue;
            deny;
            deny_and_continue;
        }
        default_action = deny;
    }
    @name("stage2:dash_acl_rule|dash_acl") table stage2 {
        key = {
            meta.stage2_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
        }
        actions = {
            permit;
            permit_and_continue;
            deny;
            deny_and_continue;
        }
        default_action = deny;
    }
    @name("stage3:dash_acl_rule|dash_acl") table stage3 {
        key = {
            meta.stage3_dash_acl_group_id: exact @name("meta.dash_acl_group_id:dash_acl_group_id");
            meta.dst_ip_addr             : ternary @name("meta.dst_ip_addr:dip");
            meta.src_ip_addr             : ternary @name("meta.src_ip_addr:sip");
            meta.ip_protocol             : ternary @name("meta.ip_protocol:protocol");
            meta.src_l4_port             : range @name("meta.src_l4_port:src_port");
            meta.dst_l4_port             : range @name("meta.dst_l4_port:dst_port");
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

action service_tunnel_encode(inout headers_t hdr, in IPv6Address st_dst, in IPv6Address st_dst_mask, in IPv6Address st_src, in IPv6Address st_src_mask) {
    hdr.ipv6.setValid();
    hdr.ipv6.version = 6;
    hdr.ipv6.traffic_class = 0;
    hdr.ipv6.flow_label = 0;
    hdr.ipv6.payload_length = hdr.ipv4.total_len - IPV4_HDR_SIZE;
    hdr.ipv6.next_header = hdr.ipv4.protocol;
    hdr.ipv6.hop_limit = hdr.ipv4.ttl;
    hdr.ipv6.dst_addr = (IPv6Address)hdr.ipv4.dst_addr & ~st_dst_mask | st_dst & st_dst_mask;
    hdr.ipv6.src_addr = (IPv6Address)hdr.ipv4.src_addr & ~st_src_mask | st_src & st_src_mask;
    hdr.ipv4.setInvalid();
    hdr.ethernet.ether_type = 0x86dd;
}
action service_tunnel_decode(inout headers_t hdr, in IPv4Address src, in IPv4Address dst) {
    hdr.ipv4.setValid();
    hdr.ipv4.version = 4;
    hdr.ipv4.ihl = 5;
    hdr.ipv4.diffserv = 0;
    hdr.ipv4.total_len = hdr.ipv6.payload_length + IPV4_HDR_SIZE;
    hdr.ipv4.identification = 1;
    hdr.ipv4.flags = 0;
    hdr.ipv4.frag_offset = 0;
    hdr.ipv4.protocol = hdr.ipv6.next_header;
    hdr.ipv4.ttl = hdr.ipv6.hop_limit;
    hdr.ipv4.hdr_checksum = 0;
    hdr.ipv4.dst_addr = dst;
    hdr.ipv4.src_addr = src;
    hdr.ipv6.setInvalid();
    hdr.ethernet.ether_type = 0x800;
}
control outbound(inout headers_t hdr, inout metadata_t meta) {
    action set_route_meter_attrs(bit<1> meter_policy_en, bit<16> meter_class) {
        meta.meter_policy_en = meter_policy_en;
        meta.route_meter_class = meter_class;
    }
    action route_vnet(bit<16> dst_vnet_id, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.dst_vnet_id = dst_vnet_id;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action route_vnet_direct(bit<16> dst_vnet_id, bit<1> is_overlay_ip_v4_or_v6, IPv4ORv6Address overlay_ip, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.dst_vnet_id = dst_vnet_id;
        meta.lkup_dst_ip_addr = overlay_ip;
        meta.is_lkup_dst_ip_v6 = is_overlay_ip_v4_or_v6;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action route_direct(bit<1> meter_policy_en, bit<16> meter_class) {
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    action drop() {
        meta.dropped = true;
    }
    action route_service_tunnel(bit<1> is_overlay_dip_v4_or_v6, IPv4ORv6Address overlay_dip, bit<1> is_overlay_dip_mask_v4_or_v6, IPv4ORv6Address overlay_dip_mask, bit<1> is_overlay_sip_v4_or_v6, IPv4ORv6Address overlay_sip, bit<1> is_overlay_sip_mask_v4_or_v6, IPv4ORv6Address overlay_sip_mask, bit<1> is_underlay_dip_v4_or_v6, IPv4ORv6Address underlay_dip, bit<1> is_underlay_sip_v4_or_v6, IPv4ORv6Address underlay_sip, dash_encapsulation_t dash_encapsulation, bit<24> tunnel_key, bit<1> meter_policy_en, bit<16> meter_class) {
        meta.encap_data.original_overlay_dip = hdr.ipv4.src_addr;
        meta.encap_data.original_overlay_sip = hdr.ipv4.dst_addr;
        service_tunnel_encode(hdr, overlay_dip, overlay_dip_mask, overlay_sip, overlay_sip_mask);
        meta.encap_data.underlay_dip = (underlay_dip == 0 ? meta.encap_data.original_overlay_dip : (IPv4Address)underlay_dip);
        meta.encap_data.underlay_sip = (underlay_sip == 0 ? meta.encap_data.original_overlay_sip : (IPv4Address)underlay_sip);
        meta.encap_data.overlay_dmac = hdr.ethernet.dst_addr;
        meta.encap_data.dash_encapsulation = dash_encapsulation;
        meta.encap_data.service_tunnel_key = tunnel_key;
        set_route_meter_attrs(meter_policy_en, meter_class);
    }
    @name("outbound_routing|dash_outbound_routing") table routing {
        key = {
            meta.eni_id          : exact @name("meta.eni_id:eni_id");
            meta.is_overlay_ip_v6: exact @name("meta.is_overlay_ip_v6:is_destination_v4_or_v6");
            meta.dst_ip_addr     : lpm @name("meta.dst_ip_addr:destination");
        }
        actions = {
            route_vnet;
            route_vnet_direct;
            route_direct;
            route_service_tunnel;
            drop;
        }
        const default_action = drop;
    }
    action set_tunnel_mapping(IPv4Address underlay_dip, EthernetAddress overlay_dmac, bit<1> use_dst_vnet_vni, bit<16> meter_class, bit<1> meter_class_override) {
        if (use_dst_vnet_vni == 1) {
            meta.vnet_id = meta.dst_vnet_id;
        }
        meta.encap_data.overlay_dmac = overlay_dmac;
        meta.encap_data.underlay_dip = underlay_dip;
        meta.mapping_meter_class = meter_class;
        meta.mapping_meter_class_override = meter_class_override;
    }
    @name("outbound_ca_to_pa|dash_outbound_ca_to_pa") table ca_to_pa {
        key = {
            meta.dst_vnet_id      : exact @name("meta.dst_vnet_id:dst_vnet_id");
            meta.is_lkup_dst_ip_v6: exact @name("meta.is_lkup_dst_ip_v6:is_dip_v4_or_v6");
            meta.lkup_dst_ip_addr : exact @name("meta.lkup_dst_ip_addr:dip");
        }
        actions = {
            set_tunnel_mapping;
            @defaultonly drop;
        }
        const default_action = drop;
    }
    action set_vnet_attrs(bit<24> vni) {
        meta.encap_data.vni = vni;
    }
    @name("vnet|dash_vnet") table vnet {
        key = {
            meta.vnet_id: exact @name("meta.vnet_id:vnet_id");
        }
        actions = {
            set_vnet_attrs;
        }
    }
    apply {
        if (!meta.conntrack_data.allow_out) {
            acl.apply(hdr, meta);
        }
        meta.lkup_dst_ip_addr = meta.dst_ip_addr;
        meta.is_lkup_dst_ip_v6 = meta.is_overlay_ip_v6;
        switch (routing.apply().action_run) {
            route_vnet_direct: 
            route_vnet: {
                ca_to_pa.apply();
                vnet.apply();
                vxlan_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, meta.encap_data.overlay_dmac, meta.encap_data.vni);
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
        }
    }
}

control inbound(inout headers_t hdr, inout metadata_t meta) {
    apply {
        if (!meta.conntrack_data.allow_in) {
            acl.apply(hdr, meta);
        }
        vxlan_encap(hdr, meta.encap_data.underlay_dmac, meta.encap_data.underlay_smac, meta.encap_data.underlay_dip, meta.encap_data.underlay_sip, hdr.ethernet.dst_addr, meta.encap_data.vni);
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
    @name("vip|dash_vip") table vip {
        key = {
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr:VIP");
        }
        actions = {
            accept;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action set_outbound_direction() {
        meta.direction = dash_direction_t.OUTBOUND;
    }
    action set_inbound_direction() {
        meta.direction = dash_direction_t.INBOUND;
    }
    @name("direction_lookup|dash_direction_lookup") table direction_lookup {
        key = {
            hdr.vxlan.vni: exact @name("hdr.vxlan.vni:VNI");
        }
        actions = {
            set_outbound_direction;
            @defaultonly set_inbound_direction;
        }
        const default_action = set_inbound_direction;
    }
    action set_appliance(EthernetAddress neighbor_mac, EthernetAddress mac) {
        meta.encap_data.underlay_dmac = neighbor_mac;
        meta.encap_data.underlay_smac = mac;
    }
    table appliance {
        key = {
            meta.appliance_id: ternary @name("meta.appliance_id:appliance_id");
        }
        actions = {
            set_appliance;
        }
    }
    action set_eni_attrs(bit<32> cps, bit<32> pps, bit<32> flows, bit<1> admin_state, IPv4Address vm_underlay_dip, @Sai[type="sai_uint32_t"] bit<24> vm_vni, bit<16> vnet_id, bit<16> v4_meter_policy_id, bit<16> v6_meter_policy_id, bit<16> inbound_v4_stage1_dash_acl_group_id, bit<16> inbound_v4_stage2_dash_acl_group_id, bit<16> inbound_v4_stage3_dash_acl_group_id, bit<16> inbound_v4_stage4_dash_acl_group_id, bit<16> inbound_v4_stage5_dash_acl_group_id, bit<16> inbound_v6_stage1_dash_acl_group_id, bit<16> inbound_v6_stage2_dash_acl_group_id, bit<16> inbound_v6_stage3_dash_acl_group_id, bit<16> inbound_v6_stage4_dash_acl_group_id, bit<16> inbound_v6_stage5_dash_acl_group_id, bit<16> outbound_v4_stage1_dash_acl_group_id, bit<16> outbound_v4_stage2_dash_acl_group_id, bit<16> outbound_v4_stage3_dash_acl_group_id, bit<16> outbound_v4_stage4_dash_acl_group_id, bit<16> outbound_v4_stage5_dash_acl_group_id, bit<16> outbound_v6_stage1_dash_acl_group_id, bit<16> outbound_v6_stage2_dash_acl_group_id, bit<16> outbound_v6_stage3_dash_acl_group_id, bit<16> outbound_v6_stage4_dash_acl_group_id, bit<16> outbound_v6_stage5_dash_acl_group_id) {
        meta.eni_data.cps = cps;
        meta.eni_data.pps = pps;
        meta.eni_data.flows = flows;
        meta.eni_data.admin_state = admin_state;
        meta.encap_data.underlay_dip = vm_underlay_dip;
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
    }
    @name("eni|dash_eni") table eni {
        key = {
            meta.eni_id: exact @name("meta.eni_id:eni_id");
        }
        actions = {
            set_eni_attrs;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    table eni_meter {
        key = {
            meta.eni_id   : exact @name("meta.eni_id:eni_id");
            meta.direction: exact @name("meta.direction:direction");
            meta.dropped  : exact @name("meta.dropped:dropped");
        }
        actions = {
            NoAction;
        }
    }
    action permit() {
    }
    action vxlan_decap_pa_validate(bit<16> src_vnet_id) {
        meta.vnet_id = src_vnet_id;
    }
    @name("pa_validation|dash_pa_validation") table pa_validation {
        key = {
            meta.vnet_id     : exact @name("meta.vnet_id:vnet_id");
            hdr.ipv4.src_addr: exact @name("hdr.ipv4.src_addr:sip");
        }
        actions = {
            permit;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    @name("inbound_routing|dash_inbound_routing") table inbound_routing {
        key = {
            meta.eni_id      : exact @name("meta.eni_id:eni_id");
            hdr.vxlan.vni    : exact @name("hdr.vxlan.vni:VNI");
            hdr.ipv4.src_addr: ternary @name("hdr.ipv4.src_addr:sip");
        }
        actions = {
            vxlan_decap(hdr);
            vxlan_decap_pa_validate;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action check_ip_addr_family(@Sai[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
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
    @name("meter_policy|dash_meter") @Sai[isobject="true"] table meter_policy {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id:meter_policy_id");
        }
        actions = {
            check_ip_addr_family;
        }
    }
    action set_policy_meter_class(bit<16> meter_class) {
        meta.policy_meter_class = meter_class;
    }
    @name("meter_rule|dash_meter") @Sai[isobject="true"] table meter_rule {
        key = {
            meta.meter_policy_id: exact @name("meta.meter_policy_id:meter_policy_id") @Sai[type="sai_object_id_t", isresourcetype="true", objects="METER_POLICY"];
            hdr.ipv4.dst_addr   : ternary @name("hdr.ipv4.dst_addr:dip");
        }
        actions = {
            set_policy_meter_class;
            @defaultonly NoAction;
        }
        const default_action = NoAction();
    }
    action meter_bucket_action(@Sai[type="sai_uint64_t", isreadonly="true"] bit<64> outbound_bytes_counter, @Sai[type="sai_uint64_t", isreadonly="true"] bit<64> inbound_bytes_counter, @Sai[type="sai_uint32_t", skipattr="true"] bit<32> meter_bucket_index) {
        meta.meter_bucket_index = meter_bucket_index;
    }
    @name("meter_bucket|dash_meter") @Sai[isobject="true"] table meter_bucket {
        key = {
            meta.eni_id     : exact @name("meta.eni_id:eni_id");
            meta.meter_class: exact @name("meta.meter_class:meter_class");
        }
        actions = {
            meter_bucket_action;
            @defaultonly NoAction;
        }
        const default_action = NoAction();
    }
    action set_eni(bit<16> eni_id) {
        meta.eni_id = eni_id;
    }
    @name("eni_ether_address_map|dash_eni") table eni_ether_address_map {
        key = {
            meta.eni_addr: exact @name("meta.eni_addr:address");
        }
        actions = {
            set_eni;
            @defaultonly deny;
        }
        const default_action = deny;
    }
    action set_acl_group_attrs(@Sai[type="sai_ip_addr_family_t", isresourcetype="true"] bit<32> ip_addr_family) {
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
    @name("dash_acl_group|dash_acl") table acl_group {
        key = {
            meta.stage1_dash_acl_group_id: exact @name("meta.stage1_dash_acl_group_id:dash_acl_group_id");
        }
        actions = {
            set_acl_group_attrs();
        }
    }
    action set_src_tag(tag_map_t tag_map) {
        meta.src_tag_map = tag_map;
    }
    @name("src_tag|dash_tag") table src_tag {
        key = {
            meta.src_ip_addr: lpm @name("meta.src_ip_addr:sip");
        }
        actions = {
            set_src_tag;
        }
    }
    action set_dst_tag(tag_map_t tag_map) {
        meta.dst_tag_map = tag_map;
    }
    @name("dst_tag|dash_tag") table dst_tag {
        key = {
            meta.dst_ip_addr: lpm @name("meta.dst_ip_addr:dip");
        }
        actions = {
            set_dst_tag;
        }
    }
    apply {
        if (vip.apply().hit) {
            meta.encap_data.underlay_sip = hdr.ipv4.dst_addr;
        }
        direction_lookup.apply();
        appliance.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            vxlan_decap(hdr);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            switch (inbound_routing.apply().action_run) {
                vxlan_decap_pa_validate: {
                    pa_validation.apply();
                    vxlan_decap(hdr);
                }
            }
        }
        meta.is_overlay_ip_v6 = 0;
        meta.ip_protocol = 0;
        meta.dst_ip_addr = 0;
        meta.src_ip_addr = 0;
        if (hdr.ipv6.isValid()) {
            meta.ip_protocol = hdr.ipv6.next_header;
            meta.src_ip_addr = hdr.ipv6.src_addr;
            meta.dst_ip_addr = hdr.ipv6.dst_addr;
            meta.is_overlay_ip_v6 = 1;
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
        meta.eni_addr = (meta.direction == dash_direction_t.OUTBOUND ? hdr.ethernet.src_addr : hdr.ethernet.dst_addr);
        eni_ether_address_map.apply();
        eni.apply();
        if (meta.eni_data.admin_state == 0) {
            deny();
        }
        acl_group.apply();
        src_tag.apply();
        dst_tag.apply();
        if (meta.direction == dash_direction_t.OUTBOUND) {
            outbound.apply(hdr, meta);
        } else if (meta.direction == dash_direction_t.INBOUND) {
            inbound.apply(hdr, meta);
        }
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
        } else if (meta.direction == dash_direction_t.INBOUND) {
        }
        eni_meter.apply();
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
