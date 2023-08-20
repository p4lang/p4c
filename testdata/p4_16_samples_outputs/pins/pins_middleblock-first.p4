#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> ethernet_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<128> ipv6_addr_t;
header ethernet_t {
    ethernet_addr_t dst_addr;
    ethernet_addr_t src_addr;
    bit<16>         ether_type;
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

enum bit<8> PreservedFieldList {
    CLONE_I2E = 8w1
}

type bit<10> nexthop_id_t;
type bit<10> tunnel_id_t;
type bit<12> wcmp_group_id_t;
type bit<10> vrf_id_t;
const vrf_id_t kDefaultVrf = (vrf_id_t)10w0;
type bit<10> router_interface_id_t;
type bit<9> port_id_t;
type bit<10> mirror_session_id_t;
type bit<8> qos_queue_t;
typedef bit<6> route_metadata_t;
enum bit<2> MeterColor_t {
    GREEN = 2w0,
    YELLOW = 2w1,
    RED = 2w2
}

struct headers_t {
    ethernet_t erspan_ethernet;
    ipv4_t     erspan_ipv4;
    gre_t      erspan_gre;
    ethernet_t ethernet;
    ipv6_t     tunnel_encap_ipv6;
    gre_t      tunnel_encap_gre;
    ipv4_t     ipv4;
    ipv6_t     ipv6;
    icmp_t     icmp;
    tcp_t      tcp;
    udp_t      udp;
    arp_t      arp;
}

struct packet_rewrites_t {
    ethernet_addr_t src_mac;
    ethernet_addr_t dst_mac;
}

struct local_metadata_t {
    bool                admit_to_l3;
    vrf_id_t            vrf_id;
    packet_rewrites_t   packet_rewrites;
    bit<16>             l4_src_port;
    bit<16>             l4_dst_port;
    bit<16>             wcmp_selector_input;
    bool                apply_tunnel_encap_at_egress;
    ipv6_addr_t         tunnel_encap_src_ipv6;
    ipv6_addr_t         tunnel_encap_dst_ipv6;
    bool                mirror_session_id_valid;
    mirror_session_id_t mirror_session_id_value;
    @field_list(PreservedFieldList.CLONE_I2E)
    ipv4_addr_t         mirroring_src_ip;
    @field_list(PreservedFieldList.CLONE_I2E)
    ipv4_addr_t         mirroring_dst_ip;
    @field_list(PreservedFieldList.CLONE_I2E)
    ethernet_addr_t     mirroring_src_mac;
    @field_list(PreservedFieldList.CLONE_I2E)
    ethernet_addr_t     mirroring_dst_mac;
    @field_list(PreservedFieldList.CLONE_I2E)
    bit<8>              mirroring_ttl;
    @field_list(PreservedFieldList.CLONE_I2E)
    bit<8>              mirroring_tos;
    MeterColor_t        color;
    port_id_t           ingress_port;
    route_metadata_t    route_metadata;
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
    @id(3)
    bit<7>    unused_pad;
}

parser packet_parser(packet_in packet, out headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    state start {
        local_metadata.admit_to_l3 = false;
        local_metadata.vrf_id = (vrf_id_t)10w0;
        local_metadata.packet_rewrites.src_mac = 48w0;
        local_metadata.packet_rewrites.dst_mac = 48w0;
        local_metadata.l4_src_port = 16w0;
        local_metadata.l4_dst_port = 16w0;
        local_metadata.wcmp_selector_input = 16w0;
        local_metadata.mirror_session_id_valid = false;
        local_metadata.color = MeterColor_t.GREEN;
        local_metadata.ingress_port = (port_id_t)standard_metadata.ingress_port;
        local_metadata.route_metadata = 6w0;
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(headers.ethernet);
        transition select(headers.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            16w0x86dd: parse_ipv6;
            16w0x806: parse_arp;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(headers.ipv4);
        transition select(headers.ipv4.protocol) {
            8w0x1: parse_icmp;
            8w0x6: parse_tcp;
            8w0x11: parse_udp;
            default: accept;
        }
    }
    state parse_ipv6 {
        packet.extract<ipv6_t>(headers.ipv6);
        transition select(headers.ipv6.next_header) {
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
        packet.emit<ethernet_t>(headers.erspan_ethernet);
        packet.emit<ipv4_t>(headers.erspan_ipv4);
        packet.emit<gre_t>(headers.erspan_gre);
        packet.emit<ethernet_t>(headers.ethernet);
        packet.emit<ipv6_t>(headers.tunnel_encap_ipv6);
        packet.emit<gre_t>(headers.tunnel_encap_gre);
        packet.emit<ipv4_t>(headers.ipv4);
        packet.emit<ipv6_t>(headers.ipv6);
        packet.emit<arp_t>(headers.arp);
        packet.emit<icmp_t>(headers.icmp);
        packet.emit<tcp_t>(headers.tcp);
        packet.emit<udp_t>(headers.udp);
    }
}

control routing(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bool wcmp_group_id_valid = false;
    wcmp_group_id_t wcmp_group_id_value;
    bool nexthop_id_valid = false;
    nexthop_id_t nexthop_id_value;
    bool router_interface_id_valid = false;
    router_interface_id_t router_interface_id_value;
    bool neighbor_id_valid = false;
    ipv6_addr_t neighbor_id_value;
    @id(0x01000001) action set_dst_mac(@id(1) @format(MAC_ADDRESS) ethernet_addr_t dst_mac) {
        local_metadata.packet_rewrites.dst_mac = dst_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x02000040) table neighbor_table {
        key = {
            router_interface_id_value: exact @id(1) @name("router_interface_id") @refers_to(router_interface_table , router_interface_id);
            neighbor_id_value        : exact @id(2) @format(IPV6_ADDRESS) @name("neighbor_id");
        }
        actions = {
            @proto_id(1) set_dst_mac();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 1024;
    }
    @id(0x01000002) action set_port_and_src_mac(@id(1) port_id_t port, @id(2) @format(MAC_ADDRESS) ethernet_addr_t src_mac) {
        standard_metadata.egress_spec = (bit<9>)port;
        local_metadata.packet_rewrites.src_mac = src_mac;
    }
    @p4runtime_role("sdn_controller") @id(0x02000041) table router_interface_table {
        key = {
            router_interface_id_value: exact @id(1) @name("router_interface_id");
        }
        actions = {
            @proto_id(1) set_port_and_src_mac();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 256;
    }
    @id(0x01000014) action set_ip_nexthop(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        router_interface_id_valid = true;
        router_interface_id_value = router_interface_id;
        neighbor_id_valid = true;
        neighbor_id_value = neighbor_id;
    }
    @id(0x01000003) @deprecated("Use set_ip_nexthop instead.") action set_nexthop(@id(1) @refers_to(router_interface_table , router_interface_id) @refers_to(neighbor_table , router_interface_id) router_interface_id_t router_interface_id, @id(2) @format(IPV6_ADDRESS) @refers_to(neighbor_table , neighbor_id) ipv6_addr_t neighbor_id) {
        set_ip_nexthop(router_interface_id, neighbor_id);
    }
    @p4runtime_role("sdn_controller") @id(0x02000042) table nexthop_table {
        key = {
            nexthop_id_value: exact @id(1) @name("nexthop_id");
        }
        actions = {
            @proto_id(1) set_nexthop();
            @proto_id(3) set_ip_nexthop();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 1024;
    }
    @id(0x01000005) action set_nexthop_id(@id(1) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id) {
        nexthop_id_valid = true;
        nexthop_id_value = nexthop_id;
    }
    @id(0x01000010) action set_nexthop_id_and_metadata(@id(1) @refers_to(nexthop_table , nexthop_id) nexthop_id_t nexthop_id, route_metadata_t route_metadata) {
        nexthop_id_valid = true;
        nexthop_id_value = nexthop_id;
        local_metadata.route_metadata = route_metadata;
    }
    @max_group_size(256) action_selector(HashAlgorithm.identity, 32w65536, 32w16) wcmp_group_selector;
    @p4runtime_role("sdn_controller") @id(0x02000043) @oneshot table wcmp_group_table {
        key = {
            wcmp_group_id_value               : exact @id(1) @name("wcmp_group_id");
            local_metadata.wcmp_selector_input: selector @name("local_metadata.wcmp_selector_input");
        }
        actions = {
            @proto_id(1) set_nexthop_id();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        @id(0x11DC4EC8) implementation = wcmp_group_selector;
        size = 3968;
    }
    action no_action() {
    }
    @entry_restriction("
    // The VRF ID 0 (or '' in P4Runtime) encodes the default VRF, which cannot
    // be read or written via this table, but is always present implicitly.
    // TODO: This constraint should read `vrf_id != ''` (since
    // constraints are a control plane (P4Runtime) concept), but
    // p4-constraints does not currently support strings.
    vrf_id != 0;
  ") @p4runtime_role("sdn_controller") @id(0x0200004A) table vrf_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id");
        }
        actions = {
            @proto_id(1) no_action();
        }
        const default_action = no_action();
        size = 64;
    }
    @id(0x01000006) action drop() {
        mark_to_drop(standard_metadata);
    }
    @id(0x01000004) action set_wcmp_group_id(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id) {
        wcmp_group_id_valid = true;
        wcmp_group_id_value = wcmp_group_id;
    }
    @id(0x01000011) action set_wcmp_group_id_and_metadata(@id(1) @refers_to(wcmp_group_table , wcmp_group_id) wcmp_group_id_t wcmp_group_id, route_metadata_t route_metadata) {
        set_wcmp_group_id(wcmp_group_id);
        local_metadata.route_metadata = route_metadata;
    }
    @id(0x0100000F) action trap() {
        clone(CloneType.I2E, 32w1024);
        mark_to_drop(standard_metadata);
    }
    @p4runtime_role("sdn_controller") @id(0x02000044) table ipv4_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv4.dst_addr: lpm @format(IPV4_ADDRESS) @id(2) @name("ipv4_dst");
        }
        actions = {
            @proto_id(1) drop();
            @proto_id(2) set_nexthop_id();
            @proto_id(3) set_wcmp_group_id();
            @proto_id(4) trap();
            @proto_id(5) set_nexthop_id_and_metadata();
            @proto_id(6) set_wcmp_group_id_and_metadata();
        }
        const default_action = drop();
        size = 32768;
    }
    @p4runtime_role("sdn_controller") @id(0x02000045) table ipv6_table {
        key = {
            local_metadata.vrf_id: exact @id(1) @name("vrf_id") @refers_to(vrf_table , vrf_id);
            headers.ipv6.dst_addr: lpm @format(IPV6_ADDRESS) @id(2) @name("ipv6_dst");
        }
        actions = {
            @proto_id(1) drop();
            @proto_id(2) set_nexthop_id();
            @proto_id(3) set_wcmp_group_id();
            @proto_id(4) trap();
            @proto_id(5) set_nexthop_id_and_metadata();
            @proto_id(6) set_wcmp_group_id_and_metadata();
        }
        const default_action = drop();
        size = 4096;
    }
    apply {
        mark_to_drop(standard_metadata);
        vrf_table.apply();
        if (local_metadata.admit_to_l3) {
            if (headers.ipv4.isValid()) {
                ipv4_table.apply();
            } else if (headers.ipv6.isValid()) {
                ipv6_table.apply();
            }
            if (wcmp_group_id_valid) {
                wcmp_group_table.apply();
            }
            if (nexthop_id_valid) {
                nexthop_table.apply();
                if (router_interface_id_valid && neighbor_id_valid) {
                    router_interface_table.apply();
                    neighbor_table.apply();
                }
            }
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
        update_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<1>, bit<1>, bit<1>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(headers.erspan_ipv4.isValid(), { headers.erspan_ipv4.version, headers.erspan_ipv4.ihl, headers.erspan_ipv4.dscp, headers.erspan_ipv4.ecn, headers.erspan_ipv4.total_len, headers.erspan_ipv4.identification, headers.erspan_ipv4.reserved, headers.erspan_ipv4.do_not_fragment, headers.erspan_ipv4.more_fragments, headers.erspan_ipv4.frag_offset, headers.erspan_ipv4.ttl, headers.erspan_ipv4.protocol, headers.erspan_ipv4.src_addr, headers.erspan_ipv4.dst_addr }, headers.erspan_ipv4.header_checksum, HashAlgorithm.csum16);
        update_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<1>, bit<1>, bit<1>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(headers.ipv4.isValid(), { headers.ipv4.version, headers.ipv4.ihl, headers.ipv4.dscp, headers.ipv4.ecn, headers.ipv4.total_len, headers.ipv4.identification, headers.ipv4.reserved, headers.ipv4.do_not_fragment, headers.ipv4.more_fragments, headers.ipv4.frag_offset, headers.ipv4.ttl, headers.ipv4.protocol, headers.ipv4.src_addr, headers.ipv4.dst_addr }, headers.ipv4.header_checksum, HashAlgorithm.csum16);
    }
}

control mirroring_encap(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (standard_metadata.instance_type == 32w1) {
            headers.erspan_ethernet.setValid();
            headers.erspan_ethernet.src_addr = local_metadata.mirroring_src_mac;
            headers.erspan_ethernet.dst_addr = local_metadata.mirroring_dst_mac;
            headers.erspan_ethernet.ether_type = 16w0x800;
            headers.erspan_ipv4.setValid();
            headers.erspan_ipv4.src_addr = local_metadata.mirroring_src_ip;
            headers.erspan_ipv4.dst_addr = local_metadata.mirroring_dst_ip;
            headers.erspan_ipv4.version = 4w4;
            headers.erspan_ipv4.ihl = 4w5;
            headers.erspan_ipv4.protocol = 8w0x2f;
            headers.erspan_ipv4.ttl = local_metadata.mirroring_ttl;
            headers.erspan_ipv4.dscp = local_metadata.mirroring_tos[7:2];
            headers.erspan_ipv4.ecn = local_metadata.mirroring_tos[1:0];
            headers.erspan_ipv4.total_len = 16w24 + (bit<16>)standard_metadata.packet_length;
            headers.erspan_ipv4.identification = 16w0;
            headers.erspan_ipv4.reserved = 1w0;
            headers.erspan_ipv4.do_not_fragment = 1w1;
            headers.erspan_ipv4.more_fragments = 1w0;
            headers.erspan_ipv4.frag_offset = 13w0;
            headers.erspan_ipv4.header_checksum = 16w0;
            headers.erspan_gre.setValid();
            headers.erspan_gre.checksum_present = 1w0;
            headers.erspan_gre.routing_present = 1w0;
            headers.erspan_gre.key_present = 1w0;
            headers.erspan_gre.sequence_present = 1w0;
            headers.erspan_gre.strict_source_route = 1w0;
            headers.erspan_gre.recursion_control = 3w0;
            headers.erspan_gre.acknowledgement_present = 1w0;
            headers.erspan_gre.flags = 4w0;
            headers.erspan_gre.version = 3w0;
            headers.erspan_gre.protocol = 16w0x88be;
        }
    }
}

control mirroring_clone(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    port_id_t mirror_port;
    bit<32> pre_session;
    @id(0x01000007) action mirror_as_ipv4_erspan(@id(1) port_id_t port, @id(2) @format(IPV4_ADDRESS) ipv4_addr_t src_ip, @id(3) @format(IPV4_ADDRESS) ipv4_addr_t dst_ip, @id(4) @format(MAC_ADDRESS) ethernet_addr_t src_mac, @id(5) @format(MAC_ADDRESS) ethernet_addr_t dst_mac, @id(6) bit<8> ttl, @id(7) bit<8> tos) {
        mirror_port = port;
        local_metadata.mirroring_src_ip = src_ip;
        local_metadata.mirroring_dst_ip = dst_ip;
        local_metadata.mirroring_src_mac = src_mac;
        local_metadata.mirroring_dst_mac = dst_mac;
        local_metadata.mirroring_ttl = ttl;
        local_metadata.mirroring_tos = tos;
    }
    @p4runtime_role("sdn_controller") @id(0x02000046) table mirror_session_table {
        key = {
            local_metadata.mirror_session_id_value: exact @id(1) @name("mirror_session_id");
        }
        actions = {
            @proto_id(1) mirror_as_ipv4_erspan();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 2;
    }
    @id(0x01000009) action set_pre_session(bit<32> id) {
        pre_session = id;
    }
    @p4runtime_role("packet_replication_engine_manager") @id(0x02000048) table mirror_port_to_pre_session_table {
        key = {
            mirror_port: exact @id(1) @name("mirror_port");
        }
        actions = {
            @proto_id(1) set_pre_session();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        if (local_metadata.mirror_session_id_valid) {
            if (mirror_session_table.apply().hit) {
                if (mirror_port_to_pre_session_table.apply().hit) {
                    clone_preserving_field_list(CloneType.I2E, pre_session, 8w1);
                }
            }
        }
    }
}

control l3_admit(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    @id(0x01000008) action admit_to_l3() {
        local_metadata.admit_to_l3 = true;
    }
    @p4runtime_role("sdn_controller") @id(0x02000047) table l3_admit_table {
        key = {
            headers.ethernet.dst_addr  : ternary @name("dst_mac") @id(1) @format(MAC_ADDRESS);
            local_metadata.ingress_port: optional @name("in_port") @id(2);
        }
        actions = {
            @proto_id(1) admit_to_l3();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        size = 128;
    }
    apply {
        l3_admit_table.apply();
    }
}

control ttl(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.admit_to_l3) {
            if (headers.ipv4.isValid()) {
                if (headers.ipv4.ttl <= 8w1) {
                    mark_to_drop(standard_metadata);
                } else {
                    headers.ipv4.ttl = headers.ipv4.ttl + 8w255;
                }
            }
            if (headers.ipv6.isValid()) {
                if (headers.ipv6.hop_limit <= 8w1) {
                    mark_to_drop(standard_metadata);
                } else {
                    headers.ipv6.hop_limit = headers.ipv6.hop_limit + 8w255;
                }
            }
        }
    }
}

control packet_rewrites(inout headers_t headers, in local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    apply {
        if (local_metadata.admit_to_l3) {
            headers.ethernet.src_addr = local_metadata.packet_rewrites.src_mac;
            headers.ethernet.dst_addr = local_metadata.packet_rewrites.dst_mac;
        }
    }
}

@id(0x01000109) @sai_action(SAI_PACKET_ACTION_DROP) action acl_drop(inout standard_metadata_t standard_metadata) {
    mark_to_drop(standard_metadata);
}
control acl_ingress(in headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    bit<8> ttl = 8w0;
    bit<6> dscp = 6w0;
    bit<2> ecn = 2w0;
    bit<8> ip_protocol = 8w0;
    @id(0x15000100) direct_meter<MeterColor_t>(MeterType.bytes) acl_ingress_meter;
    @id(0x13000102) direct_counter(CounterType.packets_and_bytes) acl_ingress_counter;
    @id(0x01000101) @sai_action(SAI_PACKET_ACTION_COPY , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_YELLOW) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_RED) action acl_copy(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_ingress_counter.count();
        acl_ingress_meter.read(local_metadata.color);
        clone(CloneType.I2E, 32w1024);
    }
    @id(0x01000102) @sai_action(SAI_PACKET_ACTION_TRAP , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_YELLOW) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) action acl_trap(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_copy(qos_queue);
        mark_to_drop(standard_metadata);
    }
    @id(0x01000199) @sai_action(SAI_PACKET_ACTION_TRAP , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_YELLOW) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) action acl_experimental_trap(@sai_action_param(QOS_QUEUE) @id(1) qos_queue_t qos_queue) {
        acl_ingress_meter.read(local_metadata.color);
        acl_trap(qos_queue);
    }
    @id(0x01000103) @sai_action(SAI_PACKET_ACTION_FORWARD , SAI_PACKET_COLOR_GREEN) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_YELLOW) @sai_action(SAI_PACKET_ACTION_DROP , SAI_PACKET_COLOR_RED) action acl_forward() {
        acl_ingress_counter.count();
        acl_ingress_meter.read(local_metadata.color);
    }
    @id(0x01000104) @sai_action(SAI_PACKET_ACTION_FORWARD) action acl_mirror(@id(1) @refers_to(mirror_session_table , mirror_session_id) @sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_MIRROR_INGRESS) mirror_session_id_t mirror_session_id) {
        acl_ingress_counter.count();
        local_metadata.mirror_session_id_valid = true;
        local_metadata.mirror_session_id_value = mirror_session_id;
    }
    @p4runtime_role("sdn_controller") @id(0x02000100) @sai_acl(INGRESS) @entry_restriction("
    // Forbid using ether_type for IP packets (by convention, use is_ip* instead).
    ether_type != 0x0800 && ether_type != 0x86dd;
    // Only allow IP field matches for IP packets.
    dst_ip::mask != 0 -> is_ipv4 == 1;
    dst_ipv6::mask != 0 -> is_ipv6 == 1;
    ttl::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ecn::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    ip_protocol::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
    // Only allow l4_dst_port and l4_src_port matches for TCP/UDP packets.
    l4_src_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);
    l4_dst_port::mask != 0 -> (ip_protocol == 6 || ip_protocol == 17);

    // Only allow arp_tpa matches for ARP packets.
    arp_tpa::mask != 0 -> ether_type == 0x0806;

    // Only allow icmp_type matches for ICMP packets



    icmpv6_type::mask != 0 -> ip_protocol == 58;
    // Forbid illegal combinations of IP_TYPE fields.
    is_ip::mask != 0 -> (is_ipv4::mask == 0 && is_ipv6::mask == 0);
    is_ipv4::mask != 0 -> (is_ip::mask == 0 && is_ipv6::mask == 0);
    is_ipv6::mask != 0 -> (is_ip::mask == 0 && is_ipv4::mask == 0);
    // Forbid unsupported combinations of IP_TYPE fields.
    is_ipv4::mask != 0 -> (is_ipv4 == 1);
    is_ipv6::mask != 0 -> (is_ipv6 == 1);
  ") table acl_ingress_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @name("is_ip") @id(1) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @name("is_ipv4") @id(2) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @name("is_ipv6") @id(3) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.ether_type                     : ternary @name("ether_type") @id(4) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ETHER_TYPE);
            headers.ethernet.dst_addr                       : ternary @name("dst_mac") @id(5) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_MAC) @format(MAC_ADDRESS);
            headers.ipv4.src_addr                           : ternary @name("src_ip") @id(6) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_IP) @format(IPV4_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @name("dst_ip") @id(7) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.src_addr[127:64]                   : ternary @name("src_ipv6") @id(8) @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_SRC_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @name("dst_ipv6") @id(9) @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            ttl                                             : ternary @name("ttl") @id(10) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_TTL);
            dscp                                            : ternary @name("dscp") @id(11) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            ecn                                             : ternary @name("ecn") @id(12) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ECN);
            ip_protocol                                     : ternary @name("ip_protocol") @id(13) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IP_PROTOCOL);
            headers.icmp.type                               : ternary @name("icmpv6_type") @id(14) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ICMPV6_TYPE);
            local_metadata.l4_src_port                      : ternary @name("l4_src_port") @id(20) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_SRC_PORT);
            local_metadata.l4_dst_port                      : ternary @name("l4_dst_port") @id(15) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_L4_DST_PORT);
            headers.arp.target_proto_addr                   : ternary @name("arp_tpa") @id(16) @composite_field(@ sai_udf ( base = SAI_UDF_BASE_L3 , offset = 24 , length = 2 ) , @ sai_udf ( base = SAI_UDF_BASE_L3 , offset = 26 , length = 2 )) @format(IPV4_ADDRESS);
        }
        actions = {
            @proto_id(1) acl_copy();
            @proto_id(2) acl_trap();
            @proto_id(3) acl_forward();
            @proto_id(4) acl_mirror();
            @proto_id(5) acl_drop(standard_metadata);
            @proto_id(99) acl_experimental_trap();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        meters = acl_ingress_meter;
        counters = acl_ingress_counter;
        size = 128;
    }
    apply {
        if (headers.ipv4.isValid()) {
            ttl = headers.ipv4.ttl;
            dscp = headers.ipv4.dscp;
            ecn = headers.ipv4.ecn;
            ip_protocol = headers.ipv4.protocol;
        } else if (headers.ipv6.isValid()) {
            ttl = headers.ipv6.hop_limit;
            dscp = headers.ipv6.dscp;
            ecn = headers.ipv6.ecn;
            ip_protocol = headers.ipv6.next_header;
        }
        acl_ingress_table.apply();
    }
}

control acl_pre_ingress(in headers_t headers, inout local_metadata_t local_metadata, in standard_metadata_t standard_metadata) {
    bit<6> dscp = 6w0;
    @id(0x13000101) direct_counter(CounterType.packets_and_bytes) acl_pre_ingress_counter;
    @id(0x01000100) @sai_action(SAI_PACKET_ACTION_FORWARD) action set_vrf(@sai_action_param(SAI_ACL_ENTRY_ATTR_ACTION_SET_VRF) @refers_to(vrf_table , vrf_id) @id(1) vrf_id_t vrf_id) {
        local_metadata.vrf_id = vrf_id;
        acl_pre_ingress_counter.count();
    }
    @p4runtime_role("sdn_controller") @id(0x02000101) @sai_acl(PRE_INGRESS) @entry_restriction("
    // Only allow IP field matches for IP packets.
    dscp::mask != 0 -> (is_ip == 1 || is_ipv4 == 1 || is_ipv6 == 1);
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
  ") table acl_pre_ingress_table {
        key = {
            headers.ipv4.isValid() || headers.ipv6.isValid(): optional @name("is_ip") @id(1) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IP);
            headers.ipv4.isValid()                          : optional @name("is_ipv4") @id(2) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV4ANY);
            headers.ipv6.isValid()                          : optional @name("is_ipv6") @id(3) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_ACL_IP_TYPE / IPV6ANY);
            headers.ethernet.src_addr                       : ternary @name("src_mac") @id(4) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_SRC_MAC) @format(MAC_ADDRESS);
            headers.ipv4.dst_addr                           : ternary @name("dst_ip") @id(5) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DST_IP) @format(IPV4_ADDRESS);
            headers.ipv6.dst_addr[127:64]                   : ternary @name("dst_ipv6") @id(6) @composite_field(@ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD3 ) , @ sai_field ( SAI_ACL_TABLE_ATTR_FIELD_DST_IPV6_WORD2 )) @format(IPV6_ADDRESS);
            dscp                                            : ternary @name("dscp") @id(7) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_DSCP);
            local_metadata.ingress_port                     : optional @name("in_port") @id(8) @sai_field(SAI_ACL_TABLE_ATTR_FIELD_IN_PORT);
        }
        actions = {
            @proto_id(1) set_vrf();
            @defaultonly NoAction();
        }
        const default_action = NoAction();
        counters = acl_pre_ingress_counter;
        size = 255;
    }
    apply {
        if (headers.ipv4.isValid()) {
            dscp = headers.ipv4.dscp;
        } else if (headers.ipv6.isValid()) {
            dscp = headers.ipv6.dscp;
        }
        local_metadata.vrf_id = (vrf_id_t)10w0;
        acl_pre_ingress_table.apply();
    }
}

control admit_google_system_mac(in headers_t headers, inout local_metadata_t local_metadata) {
    apply {
        local_metadata.admit_to_l3 = headers.ethernet.dst_addr & 48w0x10000000000 == 48w0;
    }
}

control ingress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("acl_pre_ingress") acl_pre_ingress() acl_pre_ingress_inst;
    @name("admit_google_system_mac") admit_google_system_mac() admit_google_system_mac_inst;
    @name("l3_admit") l3_admit() l3_admit_inst;
    @name("routing") routing() routing_inst;
    @name("acl_ingress") acl_ingress() acl_ingress_inst;
    @name("ttl") ttl() ttl_inst;
    @name("mirroring_clone") mirroring_clone() mirroring_clone_inst;
    apply {
        acl_pre_ingress_inst.apply(headers, local_metadata, standard_metadata);
        admit_google_system_mac_inst.apply(headers, local_metadata);
        l3_admit_inst.apply(headers, local_metadata, standard_metadata);
        routing_inst.apply(headers, local_metadata, standard_metadata);
        acl_ingress_inst.apply(headers, local_metadata, standard_metadata);
        ttl_inst.apply(headers, local_metadata, standard_metadata);
        mirroring_clone_inst.apply(headers, local_metadata, standard_metadata);
    }
}

control egress(inout headers_t headers, inout local_metadata_t local_metadata, inout standard_metadata_t standard_metadata) {
    @name("packet_rewrites") packet_rewrites() packet_rewrites_inst;
    @name("mirroring_encap") mirroring_encap() mirroring_encap_inst;
    apply {
        packet_rewrites_inst.apply(headers, local_metadata, standard_metadata);
        mirroring_encap_inst.apply(headers, local_metadata, standard_metadata);
    }
}

@pkginfo(name="middleblock.p4", organization="Google") V1Switch<headers_t, local_metadata_t>(packet_parser(), verify_ipv4_checksum(), ingress(), egress(), compute_ipv4_checksum(), packet_deparser()) main;
