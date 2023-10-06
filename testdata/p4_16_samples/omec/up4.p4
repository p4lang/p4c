#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

const bit<4> IP_VERSION_4 = 4;
const bit<8> DEFAULT_IPV4_TTL = 64;
typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> l4_port_t;
typedef bit<8> ip_proto_t;
typedef bit<16> eth_type_t;
const bit<16> UDP_PORT_GTPU = 2152;
const bit<3> GTP_V1 = 0x1;
const bit<1> GTP_PROTOCOL_TYPE_GTP = 0x1;
const bit<8> GTP_MESSAGE_TYPE_UPDU = 0xff;
const bit<8> GTPU_NEXT_EXT_NONE = 0x0;
const bit<8> GTPU_NEXT_EXT_PSC = 0x85;
const bit<4> GTPU_EXT_PSC_TYPE_DL = 4w0;
const bit<4> GTPU_EXT_PSC_TYPE_UL = 4w1;
const bit<8> GTPU_EXT_PSC_LEN = 8w1;
typedef bit<32> teid_t;
typedef bit<6> qfi_t;
typedef bit<32> counter_index_t;
typedef bit<8> tunnel_peer_id_t;
typedef bit<32> app_meter_idx_t;
typedef bit<32> session_meter_idx_t;
typedef bit<6> slice_tc_meter_idx_t;
typedef bit<4> slice_id_t;
typedef bit<2> tc_t;
const qfi_t DEFAULT_QFI = 0;
const bit<8> APP_ID_UNKNOWN = 0;
const session_meter_idx_t DEFAULT_SESSION_METER_IDX = 0;
const app_meter_idx_t DEFAULT_APP_METER_IDX = 0;
enum bit<8> GTPUMessageType {
    GPDU = 255
}

enum bit<16> EtherType {
    IPV4 = 0x800,
    IPV6 = 0x86dd
}

enum bit<8> IpProtocol {
    ICMP = 1,
    TCP = 6,
    UDP = 17
}

enum bit<16> L4Port {
    DHCP_SERV = 67,
    DHCP_CLIENT = 68,
    GTP_GPDU = 2152,
    IPV4_IN_UDP = 9875
}

enum bit<8> Direction {
    UNKNOWN = 0x0,
    UPLINK = 0x1,
    DOWNLINK = 0x2,
    OTHER = 0x3
}

enum bit<8> InterfaceType {
    UNKNOWN = 0x0,
    ACCESS = 0x1,
    CORE = 0x2
}

enum bit<4> Slice {
    DEFAULT = 0x0
}

enum bit<2> TrafficClass {
    BEST_EFFORT = 0,
    CONTROL = 1,
    REAL_TIME = 2,
    ELASTIC = 3
}

enum bit<2> MeterColor {
    GREEN = 0,
    YELLOW = 1,
    RED = 2
}

header ethernet_t {
    mac_addr_t dst_addr;
    mac_addr_t src_addr;
    eth_type_t ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     total_len;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     frag_offset;
    bit<8>      ttl;
    ip_proto_t  proto;
    bit<16>     checksum;
    ipv4_addr_t src_addr;
    ipv4_addr_t dst_addr;
}

header tcp_t {
    l4_port_t sport;
    l4_port_t dport;
    bit<32>   seq_no;
    bit<32>   ack_no;
    bit<4>    data_offset;
    bit<3>    res;
    bit<3>    ecn;
    bit<6>    ctrl;
    bit<16>   window;
    bit<16>   checksum;
    bit<16>   urgent_ptr;
}

header udp_t {
    l4_port_t sport;
    l4_port_t dport;
    bit<16>   len;
    bit<16>   checksum;
}

header icmp_t {
    bit<8>  icmp_type;
    bit<8>  icmp_code;
    bit<16> checksum;
    bit<16> identifier;
    bit<16> sequence_number;
    bit<64> timestamp;
}

header gtpu_t {
    bit<3>  version;
    bit<1>  pt;
    bit<1>  spare;
    bit<1>  ex_flag;
    bit<1>  seq_flag;
    bit<1>  npdu_flag;
    bit<8>  msgtype;
    bit<16> msglen;
    teid_t  teid;
}

header gtpu_options_t {
    bit<16> seq_num;
    bit<8>  n_pdu_num;
    bit<8>  next_ext;
}

header gtpu_ext_psc_t {
    bit<8> len;
    bit<4> type;
    bit<4> spare0;
    bit<1> ppp;
    bit<1> rqi;
    bit<6> qfi;
    bit<8> next_ext;
}

@controller_header("packet_out") header packet_out_t {
    bit<8> reserved;
}

@controller_header("packet_in") header packet_in_t {
    PortId_t ingress_port;
    bit<7>   _pad;
}

struct parsed_headers_t {
    packet_out_t   packet_out;
    packet_in_t    packet_in;
    ethernet_t     ethernet;
    ipv4_t         ipv4;
    udp_t          udp;
    tcp_t          tcp;
    icmp_t         icmp;
    gtpu_t         gtpu;
    gtpu_options_t gtpu_options;
    gtpu_ext_psc_t gtpu_ext_psc;
    ipv4_t         inner_ipv4;
    udp_t          inner_udp;
    tcp_t          inner_tcp;
    icmp_t         inner_icmp;
}

struct ddn_digest_t {
    ipv4_addr_t ue_address;
}

struct local_metadata_t {
    Direction           direction;
    teid_t              teid;
    slice_id_t          slice_id;
    tc_t                tc;
    ipv4_addr_t         next_hop_ip;
    ipv4_addr_t         ue_addr;
    ipv4_addr_t         inet_addr;
    l4_port_t           ue_l4_port;
    l4_port_t           inet_l4_port;
    l4_port_t           l4_sport;
    l4_port_t           l4_dport;
    ip_proto_t          ip_proto;
    bit<8>              application_id;
    bit<8>              src_iface;
    bool                needs_gtpu_decap;
    bool                needs_tunneling;
    bool                needs_buffering;
    bool                needs_dropping;
    bool                terminations_hit;
    counter_index_t     ctr_idx;
    tunnel_peer_id_t    tunnel_peer_id;
    ipv4_addr_t         tunnel_out_src_ipv4_addr;
    ipv4_addr_t         tunnel_out_dst_ipv4_addr;
    l4_port_t           tunnel_out_udp_sport;
    teid_t              tunnel_out_teid;
    qfi_t               tunnel_out_qfi;
    session_meter_idx_t session_meter_idx_internal;
    app_meter_idx_t     app_meter_idx_internal;
    MeterColor          session_color;
    MeterColor          app_color;
    MeterColor          slice_tc_color;
    @field_list(0)
    PortId_t            preserved_ingress_port;
}

parser ParserImpl(packet_in packet, out parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    state start {
        transition select(std_meta.ingress_port) {
            255: parse_packet_out;
            default: parse_ethernet;
        }
    }
    state parse_packet_out {
        packet.extract(hdr.packet_out);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            EtherType.IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition select(hdr.ipv4.proto) {
            IpProtocol.UDP: parse_udp;
            IpProtocol.TCP: parse_tcp;
            IpProtocol.ICMP: parse_icmp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract(hdr.udp);
        local_meta.l4_sport = hdr.udp.sport;
        local_meta.l4_dport = hdr.udp.dport;
        gtpu_t gtpu = packet.lookahead<gtpu_t>();
        transition select(hdr.udp.dport, gtpu.version, gtpu.msgtype) {
            (L4Port.IPV4_IN_UDP, default, default): parse_inner_ipv4;
            (L4Port.GTP_GPDU, GTP_V1, GTPUMessageType.GPDU): parse_gtpu;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract(hdr.tcp);
        local_meta.l4_sport = hdr.tcp.sport;
        local_meta.l4_dport = hdr.tcp.dport;
        transition accept;
    }
    state parse_icmp {
        packet.extract(hdr.icmp);
        transition accept;
    }
    state parse_gtpu {
        packet.extract(hdr.gtpu);
        local_meta.teid = hdr.gtpu.teid;
        transition select(hdr.gtpu.ex_flag, hdr.gtpu.seq_flag, hdr.gtpu.npdu_flag) {
            (0, 0, 0): parse_inner_ipv4;
            default: parse_gtpu_options;
        }
    }
    state parse_gtpu_options {
        packet.extract(hdr.gtpu_options);
        bit<8> gtpu_ext_len = packet.lookahead<bit<8>>();
        transition select(hdr.gtpu_options.next_ext, gtpu_ext_len) {
            (GTPU_NEXT_EXT_PSC, GTPU_EXT_PSC_LEN): parse_gtpu_ext_psc;
            default: accept;
        }
    }
    state parse_gtpu_ext_psc {
        packet.extract(hdr.gtpu_ext_psc);
        transition select(hdr.gtpu_ext_psc.next_ext) {
            GTPU_NEXT_EXT_NONE: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.proto) {
            IpProtocol.UDP: parse_inner_udp;
            IpProtocol.TCP: parse_inner_tcp;
            IpProtocol.ICMP: parse_inner_icmp;
            default: accept;
        }
    }
    state parse_inner_udp {
        packet.extract(hdr.inner_udp);
        local_meta.l4_sport = hdr.inner_udp.sport;
        local_meta.l4_dport = hdr.inner_udp.dport;
        transition accept;
    }
    state parse_inner_tcp {
        packet.extract(hdr.inner_tcp);
        local_meta.l4_sport = hdr.inner_tcp.sport;
        local_meta.l4_dport = hdr.inner_tcp.dport;
        transition accept;
    }
    state parse_inner_icmp {
        packet.extract(hdr.inner_icmp);
        transition accept;
    }
}

control DeparserImpl(packet_out packet, in parsed_headers_t hdr) {
    apply {
        packet.emit(hdr.packet_in);
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.udp);
        packet.emit(hdr.tcp);
        packet.emit(hdr.icmp);
        packet.emit(hdr.gtpu);
        packet.emit(hdr.gtpu_options);
        packet.emit(hdr.gtpu_ext_psc);
        packet.emit(hdr.inner_ipv4);
        packet.emit(hdr.inner_udp);
        packet.emit(hdr.inner_tcp);
        packet.emit(hdr.inner_icmp);
    }
}

control VerifyChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t meta) {
    apply {
        verify_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.proto, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
        verify_checksum(hdr.inner_ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.dscp, hdr.inner_ipv4.ecn, hdr.inner_ipv4.total_len, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.frag_offset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.proto, hdr.inner_ipv4.src_addr, hdr.inner_ipv4.dst_addr }, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control ComputeChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t local_meta) {
    apply {
        update_checksum(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.proto, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
        update_checksum(hdr.inner_ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.dscp, hdr.inner_ipv4.ecn, hdr.inner_ipv4.total_len, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.frag_offset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.proto, hdr.inner_ipv4.src_addr, hdr.inner_ipv4.dst_addr }, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control Acl(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    action set_port(PortId_t port) {
        std_meta.egress_spec = port;
    }
    action punt() {
        set_port(255);
    }
    action clone_to_cpu() {
        clone_preserving_field_list(CloneType.I2E, 99, 0);
    }
    action drop() {
        mark_to_drop(std_meta);
        exit;
    }
    table acls {
        key = {
            std_meta.ingress_port  : ternary @name("inport") ;
            local_meta.src_iface   : ternary @name("src_iface") ;
            hdr.ethernet.src_addr  : ternary @name("eth_src") ;
            hdr.ethernet.dst_addr  : ternary @name("eth_dst") ;
            hdr.ethernet.ether_type: ternary @name("eth_type") ;
            hdr.ipv4.src_addr      : ternary @name("ipv4_src") ;
            hdr.ipv4.dst_addr      : ternary @name("ipv4_dst") ;
            hdr.ipv4.proto         : ternary @name("ipv4_proto") ;
            local_meta.l4_sport    : ternary @name("l4_sport") ;
            local_meta.l4_dport    : ternary @name("l4_dport") ;
        }
        actions = {
            set_port;
            punt;
            clone_to_cpu;
            drop;
            NoAction;
        }
        const default_action = NoAction;
        @name("acls") counters = direct_counter(CounterType.packets_and_bytes);
    }
    apply {
        if (hdr.ethernet.isValid() && hdr.ipv4.isValid()) {
            acls.apply();
        }
    }
}

control Routing(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    action drop() {
        mark_to_drop(std_meta);
    }
    action route(mac_addr_t src_mac, mac_addr_t dst_mac, PortId_t egress_port) {
        std_meta.egress_spec = egress_port;
        hdr.ethernet.src_addr = src_mac;
        hdr.ethernet.dst_addr = dst_mac;
    }
    table routes_v4 {
        key = {
            hdr.ipv4.dst_addr  : lpm @name("dst_prefix") ;
            hdr.ipv4.src_addr  : selector;
            hdr.ipv4.proto     : selector;
            local_meta.l4_sport: selector;
            local_meta.l4_dport: selector;
        }
        actions = {
            route;
        }
        @name("hashed_selector") implementation = action_selector(HashAlgorithm.crc16, 32w1024, 32w16);
        size = 1024;
    }
    apply {
        if (!hdr.ipv4.isValid()) {
            return;
        }
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        if (hdr.ipv4.ttl == 0) {
            drop();
        } else {
            routes_v4.apply();
        }
    }
}

control PreQosPipe(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    counter<counter_index_t>(1024, CounterType.packets_and_bytes) pre_qos_counter;
    meter<app_meter_idx_t>(1024, MeterType.bytes) app_meter;
    meter<session_meter_idx_t>(1024, MeterType.bytes) session_meter;
    meter<slice_tc_meter_idx_t>(1 << 6, MeterType.bytes) slice_tc_meter;
    action _initialize_metadata() {
        local_meta.session_meter_idx_internal = DEFAULT_SESSION_METER_IDX;
        local_meta.app_meter_idx_internal = DEFAULT_APP_METER_IDX;
        local_meta.application_id = APP_ID_UNKNOWN;
        local_meta.preserved_ingress_port = std_meta.ingress_port;
    }
    table my_station {
        key = {
            hdr.ethernet.dst_addr: exact @name("dst_mac") ;
        }
        actions = {
            NoAction;
        }
    }
    action set_source_iface(InterfaceType src_iface, Direction direction, Slice slice_id) {
        local_meta.src_iface = src_iface;
        local_meta.direction = direction;
        local_meta.slice_id = slice_id;
    }
    table interfaces {
        key = {
            hdr.ipv4.dst_addr: lpm @name("ipv4_dst_prefix") ;
        }
        actions = {
            set_source_iface;
        }
        const default_action = set_source_iface(InterfaceType.UNKNOWN, Direction.UNKNOWN, Slice.DEFAULT);
    }
    @hidden action gtpu_decap() {
        hdr.ipv4 = hdr.inner_ipv4;
        hdr.inner_ipv4.setInvalid();
        hdr.udp = hdr.inner_udp;
        hdr.inner_udp.setInvalid();
        hdr.tcp = hdr.inner_tcp;
        hdr.inner_tcp.setInvalid();
        hdr.icmp = hdr.inner_icmp;
        hdr.inner_icmp.setInvalid();
        hdr.gtpu.setInvalid();
        hdr.gtpu_options.setInvalid();
        hdr.gtpu_ext_psc.setInvalid();
    }
    @hidden action do_buffer() {
        digest<ddn_digest_t>(1, { local_meta.ue_addr });
        mark_to_drop(std_meta);
        exit;
    }
    action do_drop() {
        mark_to_drop(std_meta);
        exit;
    }
    action set_session_uplink(session_meter_idx_t session_meter_idx) {
        local_meta.needs_gtpu_decap = true;
        local_meta.session_meter_idx_internal = session_meter_idx;
    }
    action set_session_uplink_drop() {
        local_meta.needs_dropping = true;
    }
    action set_session_downlink(tunnel_peer_id_t tunnel_peer_id, session_meter_idx_t session_meter_idx) {
        local_meta.tunnel_peer_id = tunnel_peer_id;
        local_meta.session_meter_idx_internal = session_meter_idx;
    }
    action set_session_downlink_drop() {
        local_meta.needs_dropping = true;
    }
    action set_session_downlink_buff(session_meter_idx_t session_meter_idx) {
        local_meta.needs_buffering = true;
        local_meta.session_meter_idx_internal = session_meter_idx;
    }
    table sessions_uplink {
        key = {
            hdr.ipv4.dst_addr: exact @name("n3_address") ;
            local_meta.teid  : exact @name("teid") ;
        }
        actions = {
            set_session_uplink;
            set_session_uplink_drop;
            @defaultonly do_drop;
        }
        const default_action = do_drop;
    }
    table sessions_downlink {
        key = {
            hdr.ipv4.dst_addr: exact @name("ue_address") ;
        }
        actions = {
            set_session_downlink;
            set_session_downlink_drop;
            set_session_downlink_buff;
            @defaultonly do_drop;
        }
        const default_action = do_drop;
    }
    @hidden action common_term(counter_index_t ctr_idx) {
        local_meta.ctr_idx = ctr_idx;
        local_meta.terminations_hit = true;
    }
    action uplink_term_fwd(counter_index_t ctr_idx, TrafficClass tc, app_meter_idx_t app_meter_idx) {
        common_term(ctr_idx);
        local_meta.app_meter_idx_internal = app_meter_idx;
        local_meta.tc = tc;
    }
    action uplink_term_drop(counter_index_t ctr_idx) {
        common_term(ctr_idx);
        local_meta.needs_dropping = true;
    }
    action downlink_term_fwd(counter_index_t ctr_idx, teid_t teid, qfi_t qfi, TrafficClass tc, app_meter_idx_t app_meter_idx) {
        common_term(ctr_idx);
        local_meta.tunnel_out_teid = teid;
        local_meta.tunnel_out_qfi = qfi;
        local_meta.app_meter_idx_internal = app_meter_idx;
        local_meta.tc = tc;
    }
    action downlink_term_drop(counter_index_t ctr_idx) {
        common_term(ctr_idx);
        local_meta.needs_dropping = true;
    }
    table terminations_uplink {
        key = {
            local_meta.ue_addr       : exact @name("ue_address") ;
            local_meta.application_id: exact @name("app_id") ;
        }
        actions = {
            uplink_term_fwd;
            uplink_term_drop;
            @defaultonly do_drop;
        }
        const default_action = do_drop;
    }
    table terminations_downlink {
        key = {
            local_meta.ue_addr       : exact @name("ue_address") ;
            local_meta.application_id: exact @name("app_id") ;
        }
        actions = {
            downlink_term_fwd;
            downlink_term_drop;
            @defaultonly do_drop;
        }
        const default_action = do_drop;
    }
    action set_app_id(bit<8> app_id) {
        local_meta.application_id = app_id;
    }
    table applications {
        key = {
            local_meta.slice_id    : exact @name("slice_id") ;
            local_meta.inet_addr   : lpm @name("app_ip_addr") ;
            local_meta.inet_l4_port: range @name("app_l4_port") ;
            local_meta.ip_proto    : ternary @name("app_ip_proto") ;
        }
        actions = {
            set_app_id;
        }
        const default_action = set_app_id(APP_ID_UNKNOWN);
    }
    action load_tunnel_param(ipv4_addr_t src_addr, ipv4_addr_t dst_addr, l4_port_t sport) {
        local_meta.tunnel_out_src_ipv4_addr = src_addr;
        local_meta.tunnel_out_dst_ipv4_addr = dst_addr;
        local_meta.tunnel_out_udp_sport = sport;
        local_meta.needs_tunneling = true;
    }
    table tunnel_peers {
        key = {
            local_meta.tunnel_peer_id: exact @name("tunnel_peer_id") ;
        }
        actions = {
            load_tunnel_param;
        }
    }
    @hidden action _udp_encap(ipv4_addr_t src_addr, ipv4_addr_t dst_addr, l4_port_t udp_sport, L4Port udp_dport, bit<16> ipv4_total_len, bit<16> udp_len) {
        hdr.inner_udp = hdr.udp;
        hdr.udp.setInvalid();
        hdr.inner_tcp = hdr.tcp;
        hdr.tcp.setInvalid();
        hdr.inner_icmp = hdr.icmp;
        hdr.icmp.setInvalid();
        hdr.udp.setValid();
        hdr.udp.sport = udp_sport;
        hdr.udp.dport = udp_dport;
        hdr.udp.len = udp_len;
        hdr.udp.checksum = 0;
        hdr.inner_ipv4 = hdr.ipv4;
        hdr.ipv4.setValid();
        hdr.ipv4.version = IP_VERSION_4;
        hdr.ipv4.ihl = 5;
        hdr.ipv4.dscp = 0;
        hdr.ipv4.ecn = 0;
        hdr.ipv4.total_len = ipv4_total_len;
        hdr.ipv4.identification = 0x1513;
        hdr.ipv4.flags = 0;
        hdr.ipv4.frag_offset = 0;
        hdr.ipv4.ttl = DEFAULT_IPV4_TTL;
        hdr.ipv4.proto = IpProtocol.UDP;
        hdr.ipv4.src_addr = src_addr;
        hdr.ipv4.dst_addr = dst_addr;
        hdr.ipv4.checksum = 0;
    }
    @hidden action _gtpu_encap(teid_t teid) {
        hdr.gtpu.setValid();
        hdr.gtpu.version = GTP_V1;
        hdr.gtpu.pt = GTP_PROTOCOL_TYPE_GTP;
        hdr.gtpu.spare = 0;
        hdr.gtpu.ex_flag = 0;
        hdr.gtpu.seq_flag = 0;
        hdr.gtpu.npdu_flag = 0;
        hdr.gtpu.msgtype = GTPUMessageType.GPDU;
        hdr.gtpu.msglen = hdr.inner_ipv4.total_len;
        hdr.gtpu.teid = teid;
    }
    action do_gtpu_tunnel() {
        _udp_encap(local_meta.tunnel_out_src_ipv4_addr, local_meta.tunnel_out_dst_ipv4_addr, local_meta.tunnel_out_udp_sport, L4Port.GTP_GPDU, hdr.ipv4.total_len + 20 + 8 + 8, hdr.ipv4.total_len + 8 + 8);
        _gtpu_encap(local_meta.tunnel_out_teid);
    }
    action do_gtpu_tunnel_with_psc() {
        _udp_encap(local_meta.tunnel_out_src_ipv4_addr, local_meta.tunnel_out_dst_ipv4_addr, local_meta.tunnel_out_udp_sport, L4Port.GTP_GPDU, hdr.ipv4.total_len + 20 + 8 + 8 + 4 + 4, hdr.ipv4.total_len + 8 + 8 + 4 + 4);
        _gtpu_encap(local_meta.tunnel_out_teid);
        hdr.gtpu.msglen = hdr.inner_ipv4.total_len + 4 + 4;
        hdr.gtpu.ex_flag = 1;
        hdr.gtpu_options.setValid();
        hdr.gtpu_options.seq_num = 0;
        hdr.gtpu_options.n_pdu_num = 0;
        hdr.gtpu_options.next_ext = GTPU_NEXT_EXT_PSC;
        hdr.gtpu_ext_psc.setValid();
        hdr.gtpu_ext_psc.len = GTPU_EXT_PSC_LEN;
        hdr.gtpu_ext_psc.type = GTPU_EXT_PSC_TYPE_DL;
        hdr.gtpu_ext_psc.spare0 = 0;
        hdr.gtpu_ext_psc.ppp = 0;
        hdr.gtpu_ext_psc.rqi = 0;
        hdr.gtpu_ext_psc.qfi = local_meta.tunnel_out_qfi;
        hdr.gtpu_ext_psc.next_ext = GTPU_NEXT_EXT_NONE;
    }
    apply {
        _initialize_metadata();
        if (hdr.packet_out.isValid()) {
            hdr.packet_out.setInvalid();
        } else {
            if (!my_station.apply().hit) {
                return;
            }
            if (interfaces.apply().hit) {
                if (local_meta.direction == Direction.UPLINK) {
                    local_meta.ue_addr = hdr.inner_ipv4.src_addr;
                    local_meta.inet_addr = hdr.inner_ipv4.dst_addr;
                    local_meta.ue_l4_port = local_meta.l4_sport;
                    local_meta.inet_l4_port = local_meta.l4_dport;
                    local_meta.ip_proto = hdr.inner_ipv4.proto;
                    sessions_uplink.apply();
                } else if (local_meta.direction == Direction.DOWNLINK) {
                    local_meta.ue_addr = hdr.ipv4.dst_addr;
                    local_meta.inet_addr = hdr.ipv4.src_addr;
                    local_meta.ue_l4_port = local_meta.l4_dport;
                    local_meta.inet_l4_port = local_meta.l4_sport;
                    local_meta.ip_proto = hdr.ipv4.proto;
                    sessions_downlink.apply();
                    tunnel_peers.apply();
                }
                applications.apply();
                if (local_meta.direction == Direction.UPLINK) {
                    terminations_uplink.apply();
                } else if (local_meta.direction == Direction.DOWNLINK) {
                    terminations_downlink.apply();
                }
                if (local_meta.terminations_hit) {
                    pre_qos_counter.count(local_meta.ctr_idx);
                }
                app_meter.execute_meter<MeterColor>(local_meta.app_meter_idx_internal, local_meta.app_color);
                if (local_meta.app_color == MeterColor.RED) {
                    local_meta.needs_dropping = true;
                } else {
                    session_meter.execute_meter<MeterColor>(local_meta.session_meter_idx_internal, local_meta.session_color);
                    if (local_meta.session_color == MeterColor.RED) {
                        local_meta.needs_dropping = true;
                    } else {
                        slice_tc_meter.execute_meter<MeterColor>(local_meta.slice_id ++ local_meta.tc, local_meta.slice_tc_color);
                        if (local_meta.slice_tc_color == MeterColor.RED) {
                            local_meta.needs_dropping = true;
                        }
                    }
                }
                if (local_meta.needs_gtpu_decap) {
                    gtpu_decap();
                }
                if (local_meta.needs_buffering) {
                    do_buffer();
                }
                if (local_meta.needs_tunneling) {
                    if (local_meta.tunnel_out_qfi == 0) {
                        do_gtpu_tunnel();
                    } else {
                        do_gtpu_tunnel_with_psc();
                    }
                }
                if (local_meta.needs_dropping) {
                    do_drop();
                }
            }
        }
        Routing.apply(hdr, local_meta, std_meta);
        Acl.apply(hdr, local_meta, std_meta);
    }
}

control PostQosPipe(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    counter<counter_index_t>(1024, CounterType.packets_and_bytes) post_qos_counter;
    apply {
        post_qos_counter.count(local_meta.ctr_idx);
        if (std_meta.egress_port == 255) {
            hdr.packet_in.setValid();
            hdr.packet_in.ingress_port = local_meta.preserved_ingress_port;
            exit;
        }
    }
}

V1Switch(ParserImpl(), VerifyChecksumImpl(), PreQosPipe(), PostQosPipe(), ComputeChecksumImpl(), DeparserImpl()) main;

