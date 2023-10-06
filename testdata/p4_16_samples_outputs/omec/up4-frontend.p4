#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

typedef bit<48> mac_addr_t;
typedef bit<32> ipv4_addr_t;
typedef bit<16> l4_port_t;
typedef bit<8> ip_proto_t;
typedef bit<16> eth_type_t;
typedef bit<32> teid_t;
typedef bit<6> qfi_t;
typedef bit<32> counter_index_t;
typedef bit<8> tunnel_peer_id_t;
typedef bit<32> app_meter_idx_t;
typedef bit<32> session_meter_idx_t;
typedef bit<6> slice_tc_meter_idx_t;
typedef bit<4> slice_id_t;
typedef bit<2> tc_t;
enum bit<8> GTPUMessageType {
    GPDU = 8w255
}

enum bit<16> EtherType {
    IPV4 = 16w0x800,
    IPV6 = 16w0x86dd
}

enum bit<8> IpProtocol {
    ICMP = 8w1,
    TCP = 8w6,
    UDP = 8w17
}

enum bit<16> L4Port {
    DHCP_SERV = 16w67,
    DHCP_CLIENT = 16w68,
    GTP_GPDU = 16w2152,
    IPV4_IN_UDP = 16w9875
}

enum bit<8> Direction {
    UNKNOWN = 8w0x0,
    UPLINK = 8w0x1,
    DOWNLINK = 8w0x2,
    OTHER = 8w0x3
}

enum bit<8> InterfaceType {
    UNKNOWN = 8w0x0,
    ACCESS = 8w0x1,
    CORE = 8w0x2
}

enum bit<4> Slice {
    DEFAULT = 4w0x0
}

enum bit<2> TrafficClass {
    BEST_EFFORT = 2w0,
    CONTROL = 2w1,
    REAL_TIME = 2w2,
    ELASTIC = 2w3
}

enum bit<2> MeterColor {
    GREEN = 2w0,
    YELLOW = 2w1,
    RED = 2w2
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
    @name("ParserImpl.gtpu") gtpu_t gtpu_0;
    @name("ParserImpl.gtpu_ext_len") bit<8> gtpu_ext_len_0;
    state start {
        transition select(std_meta.ingress_port) {
            9w255: parse_packet_out;
            default: parse_ethernet;
        }
    }
    state parse_packet_out {
        packet.extract<packet_out_t>(hdr.packet_out);
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            EtherType.IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.proto) {
            IpProtocol.UDP: parse_udp;
            IpProtocol.TCP: parse_tcp;
            IpProtocol.ICMP: parse_icmp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        local_meta.l4_sport = hdr.udp.sport;
        local_meta.l4_dport = hdr.udp.dport;
        gtpu_0 = packet.lookahead<gtpu_t>();
        transition select(hdr.udp.dport, gtpu_0.version, gtpu_0.msgtype) {
            (L4Port.IPV4_IN_UDP, default, default): parse_inner_ipv4;
            (L4Port.GTP_GPDU, 3w0x1, GTPUMessageType.GPDU): parse_gtpu;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        local_meta.l4_sport = hdr.tcp.sport;
        local_meta.l4_dport = hdr.tcp.dport;
        transition accept;
    }
    state parse_icmp {
        packet.extract<icmp_t>(hdr.icmp);
        transition accept;
    }
    state parse_gtpu {
        packet.extract<gtpu_t>(hdr.gtpu);
        local_meta.teid = hdr.gtpu.teid;
        transition select(hdr.gtpu.ex_flag, hdr.gtpu.seq_flag, hdr.gtpu.npdu_flag) {
            (1w0, 1w0, 1w0): parse_inner_ipv4;
            default: parse_gtpu_options;
        }
    }
    state parse_gtpu_options {
        packet.extract<gtpu_options_t>(hdr.gtpu_options);
        gtpu_ext_len_0 = packet.lookahead<bit<8>>();
        transition select(hdr.gtpu_options.next_ext, gtpu_ext_len_0) {
            (8w0x85, 8w1): parse_gtpu_ext_psc;
            default: accept;
        }
    }
    state parse_gtpu_ext_psc {
        packet.extract<gtpu_ext_psc_t>(hdr.gtpu_ext_psc);
        transition select(hdr.gtpu_ext_psc.next_ext) {
            8w0x0: parse_inner_ipv4;
            default: accept;
        }
    }
    state parse_inner_ipv4 {
        packet.extract<ipv4_t>(hdr.inner_ipv4);
        transition select(hdr.inner_ipv4.proto) {
            IpProtocol.UDP: parse_inner_udp;
            IpProtocol.TCP: parse_inner_tcp;
            IpProtocol.ICMP: parse_inner_icmp;
            default: accept;
        }
    }
    state parse_inner_udp {
        packet.extract<udp_t>(hdr.inner_udp);
        local_meta.l4_sport = hdr.inner_udp.sport;
        local_meta.l4_dport = hdr.inner_udp.dport;
        transition accept;
    }
    state parse_inner_tcp {
        packet.extract<tcp_t>(hdr.inner_tcp);
        local_meta.l4_sport = hdr.inner_tcp.sport;
        local_meta.l4_dport = hdr.inner_tcp.dport;
        transition accept;
    }
    state parse_inner_icmp {
        packet.extract<icmp_t>(hdr.inner_icmp);
        transition accept;
    }
}

control DeparserImpl(packet_out packet, in parsed_headers_t hdr) {
    apply {
        packet.emit<packet_in_t>(hdr.packet_in);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<udp_t>(hdr.udp);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<icmp_t>(hdr.icmp);
        packet.emit<gtpu_t>(hdr.gtpu);
        packet.emit<gtpu_options_t>(hdr.gtpu_options);
        packet.emit<gtpu_ext_psc_t>(hdr.gtpu_ext_psc);
        packet.emit<ipv4_t>(hdr.inner_ipv4);
        packet.emit<udp_t>(hdr.inner_udp);
        packet.emit<tcp_t>(hdr.inner_tcp);
        packet.emit<icmp_t>(hdr.inner_icmp);
    }
}

control VerifyChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.proto, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
        verify_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.inner_ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.dscp, hdr.inner_ipv4.ecn, hdr.inner_ipv4.total_len, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.frag_offset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.proto, hdr.inner_ipv4.src_addr, hdr.inner_ipv4.dst_addr }, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control ComputeChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t local_meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid(), { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.dscp, hdr.ipv4.ecn, hdr.ipv4.total_len, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.frag_offset, hdr.ipv4.ttl, hdr.ipv4.proto, hdr.ipv4.src_addr, hdr.ipv4.dst_addr }, hdr.ipv4.checksum, HashAlgorithm.csum16);
        update_checksum<tuple<bit<4>, bit<4>, bit<6>, bit<2>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.inner_ipv4.isValid(), { hdr.inner_ipv4.version, hdr.inner_ipv4.ihl, hdr.inner_ipv4.dscp, hdr.inner_ipv4.ecn, hdr.inner_ipv4.total_len, hdr.inner_ipv4.identification, hdr.inner_ipv4.flags, hdr.inner_ipv4.frag_offset, hdr.inner_ipv4.ttl, hdr.inner_ipv4.proto, hdr.inner_ipv4.src_addr, hdr.inner_ipv4.dst_addr }, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control PreQosPipe(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    @name("PreQosPipe.hasReturned_0") bool hasReturned_0;
    @name("PreQosPipe.Routing.hasReturned") bool Routing_hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name("PreQosPipe.pre_qos_counter") counter<counter_index_t>(32w1024, CounterType.packets_and_bytes) pre_qos_counter_0;
    @name("PreQosPipe.app_meter") meter<app_meter_idx_t>(32w1024, MeterType.bytes) app_meter_0;
    @name("PreQosPipe.session_meter") meter<session_meter_idx_t>(32w1024, MeterType.bytes) session_meter_0;
    @name("PreQosPipe.slice_tc_meter") meter<slice_tc_meter_idx_t>(32w64, MeterType.bytes) slice_tc_meter_0;
    @name("PreQosPipe._initialize_metadata") action _initialize_metadata() {
        local_meta.session_meter_idx_internal = 32w0;
        local_meta.app_meter_idx_internal = 32w0;
        local_meta.application_id = 8w0;
        local_meta.preserved_ingress_port = std_meta.ingress_port;
    }
    @name("PreQosPipe.my_station") table my_station_0 {
        key = {
            hdr.ethernet.dst_addr: exact @name("dst_mac");
        }
        actions = {
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("PreQosPipe.set_source_iface") action set_source_iface(@name("src_iface") InterfaceType src_iface_1, @name("direction") Direction direction_1, @name("slice_id") Slice slice_id_1) {
        local_meta.src_iface = src_iface_1;
        local_meta.direction = direction_1;
        local_meta.slice_id = slice_id_1;
    }
    @name("PreQosPipe.interfaces") table interfaces_0 {
        key = {
            hdr.ipv4.dst_addr: lpm @name("ipv4_dst_prefix");
        }
        actions = {
            set_source_iface();
        }
        const default_action = set_source_iface(InterfaceType.UNKNOWN, Direction.UNKNOWN, Slice.DEFAULT);
    }
    @hidden @name("PreQosPipe.gtpu_decap") action gtpu_decap() {
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
    @hidden @name("PreQosPipe.do_buffer") action do_buffer() {
        digest<ddn_digest_t>(32w1, (ddn_digest_t){ue_address = local_meta.ue_addr});
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.do_drop") action do_drop() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.do_drop") action do_drop_1() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.do_drop") action do_drop_2() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.do_drop") action do_drop_3() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.do_drop") action do_drop_4() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.set_session_uplink") action set_session_uplink(@name("session_meter_idx") session_meter_idx_t session_meter_idx) {
        local_meta.needs_gtpu_decap = true;
        local_meta.session_meter_idx_internal = session_meter_idx;
    }
    @name("PreQosPipe.set_session_uplink_drop") action set_session_uplink_drop() {
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.set_session_downlink") action set_session_downlink(@name("tunnel_peer_id") tunnel_peer_id_t tunnel_peer_id_1, @name("session_meter_idx") session_meter_idx_t session_meter_idx_3) {
        local_meta.tunnel_peer_id = tunnel_peer_id_1;
        local_meta.session_meter_idx_internal = session_meter_idx_3;
    }
    @name("PreQosPipe.set_session_downlink_drop") action set_session_downlink_drop() {
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.set_session_downlink_buff") action set_session_downlink_buff(@name("session_meter_idx") session_meter_idx_t session_meter_idx_4) {
        local_meta.needs_buffering = true;
        local_meta.session_meter_idx_internal = session_meter_idx_4;
    }
    @name("PreQosPipe.sessions_uplink") table sessions_uplink_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("n3_address");
            local_meta.teid  : exact @name("teid");
        }
        actions = {
            set_session_uplink();
            set_session_uplink_drop();
            @defaultonly do_drop();
        }
        const default_action = do_drop();
    }
    @name("PreQosPipe.sessions_downlink") table sessions_downlink_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("ue_address");
        }
        actions = {
            set_session_downlink();
            set_session_downlink_drop();
            set_session_downlink_buff();
            @defaultonly do_drop_1();
        }
        const default_action = do_drop_1();
    }
    @name("PreQosPipe.uplink_term_fwd") action uplink_term_fwd(@name("ctr_idx") counter_index_t ctr_idx_0, @name("tc") TrafficClass tc_2, @name("app_meter_idx") app_meter_idx_t app_meter_idx) {
        @hidden {
            local_meta.ctr_idx = ctr_idx_0;
            local_meta.terminations_hit = true;
        }
        local_meta.app_meter_idx_internal = app_meter_idx;
        local_meta.tc = tc_2;
    }
    @name("PreQosPipe.uplink_term_drop") action uplink_term_drop(@name("ctr_idx") counter_index_t ctr_idx_5) {
        @hidden {
            local_meta.ctr_idx = ctr_idx_5;
            local_meta.terminations_hit = true;
        }
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.downlink_term_fwd") action downlink_term_fwd(@name("ctr_idx") counter_index_t ctr_idx_6, @name("teid") teid_t teid_1, @name("qfi") qfi_t qfi_1, @name("tc") TrafficClass tc_3, @name("app_meter_idx") app_meter_idx_t app_meter_idx_2) {
        @hidden {
            local_meta.ctr_idx = ctr_idx_6;
            local_meta.terminations_hit = true;
        }
        local_meta.tunnel_out_teid = teid_1;
        local_meta.tunnel_out_qfi = qfi_1;
        local_meta.app_meter_idx_internal = app_meter_idx_2;
        local_meta.tc = tc_3;
    }
    @name("PreQosPipe.downlink_term_drop") action downlink_term_drop(@name("ctr_idx") counter_index_t ctr_idx_7) {
        @hidden {
            local_meta.ctr_idx = ctr_idx_7;
            local_meta.terminations_hit = true;
        }
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.terminations_uplink") table terminations_uplink_0 {
        key = {
            local_meta.ue_addr       : exact @name("ue_address");
            local_meta.application_id: exact @name("app_id");
        }
        actions = {
            uplink_term_fwd();
            uplink_term_drop();
            @defaultonly do_drop_2();
        }
        const default_action = do_drop_2();
    }
    @name("PreQosPipe.terminations_downlink") table terminations_downlink_0 {
        key = {
            local_meta.ue_addr       : exact @name("ue_address");
            local_meta.application_id: exact @name("app_id");
        }
        actions = {
            downlink_term_fwd();
            downlink_term_drop();
            @defaultonly do_drop_3();
        }
        const default_action = do_drop_3();
    }
    @name("PreQosPipe.set_app_id") action set_app_id(@name("app_id") bit<8> app_id) {
        local_meta.application_id = app_id;
    }
    @name("PreQosPipe.applications") table applications_0 {
        key = {
            local_meta.slice_id    : exact @name("slice_id");
            local_meta.inet_addr   : lpm @name("app_ip_addr");
            local_meta.inet_l4_port: range @name("app_l4_port");
            local_meta.ip_proto    : ternary @name("app_ip_proto");
        }
        actions = {
            set_app_id();
        }
        const default_action = set_app_id(8w0);
    }
    @name("PreQosPipe.load_tunnel_param") action load_tunnel_param(@name("src_addr") ipv4_addr_t src_addr_1, @name("dst_addr") ipv4_addr_t dst_addr_1, @name("sport") l4_port_t sport_1) {
        local_meta.tunnel_out_src_ipv4_addr = src_addr_1;
        local_meta.tunnel_out_dst_ipv4_addr = dst_addr_1;
        local_meta.tunnel_out_udp_sport = sport_1;
        local_meta.needs_tunneling = true;
    }
    @name("PreQosPipe.tunnel_peers") table tunnel_peers_0 {
        key = {
            local_meta.tunnel_peer_id: exact @name("tunnel_peer_id");
        }
        actions = {
            load_tunnel_param();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("PreQosPipe.do_gtpu_tunnel") action do_gtpu_tunnel() {
        @hidden {
            hdr.inner_udp = hdr.udp;
            hdr.udp.setInvalid();
            hdr.inner_tcp = hdr.tcp;
            hdr.tcp.setInvalid();
            hdr.inner_icmp = hdr.icmp;
            hdr.icmp.setInvalid();
            hdr.udp.setValid();
            hdr.udp.sport = local_meta.tunnel_out_udp_sport;
            hdr.udp.dport = L4Port.GTP_GPDU;
            hdr.udp.len = hdr.ipv4.total_len + 16w16;
            hdr.udp.checksum = 16w0;
            hdr.inner_ipv4 = hdr.ipv4;
            hdr.ipv4.setValid();
            hdr.ipv4.version = 4w4;
            hdr.ipv4.ihl = 4w5;
            hdr.ipv4.dscp = 6w0;
            hdr.ipv4.ecn = 2w0;
            hdr.ipv4.total_len = hdr.ipv4.total_len + 16w36;
            hdr.ipv4.identification = 16w0x1513;
            hdr.ipv4.flags = 3w0;
            hdr.ipv4.frag_offset = 13w0;
            hdr.ipv4.ttl = 8w64;
            hdr.ipv4.proto = IpProtocol.UDP;
            hdr.ipv4.src_addr = local_meta.tunnel_out_src_ipv4_addr;
            hdr.ipv4.dst_addr = local_meta.tunnel_out_dst_ipv4_addr;
            hdr.ipv4.checksum = 16w0;
        }
        @hidden {
            hdr.gtpu.setValid();
            hdr.gtpu.version = 3w0x1;
            hdr.gtpu.pt = 1w0x1;
            hdr.gtpu.spare = 1w0;
            hdr.gtpu.ex_flag = 1w0;
            hdr.gtpu.seq_flag = 1w0;
            hdr.gtpu.npdu_flag = 1w0;
            hdr.gtpu.msgtype = GTPUMessageType.GPDU;
            hdr.gtpu.msglen = hdr.inner_ipv4.total_len;
            hdr.gtpu.teid = local_meta.tunnel_out_teid;
        }
    }
    @name("PreQosPipe.do_gtpu_tunnel_with_psc") action do_gtpu_tunnel_with_psc() {
        @hidden {
            hdr.inner_udp = hdr.udp;
            hdr.udp.setInvalid();
            hdr.inner_tcp = hdr.tcp;
            hdr.tcp.setInvalid();
            hdr.inner_icmp = hdr.icmp;
            hdr.icmp.setInvalid();
            hdr.udp.setValid();
            hdr.udp.sport = local_meta.tunnel_out_udp_sport;
            hdr.udp.dport = L4Port.GTP_GPDU;
            hdr.udp.len = hdr.ipv4.total_len + 16w24;
            hdr.udp.checksum = 16w0;
            hdr.inner_ipv4 = hdr.ipv4;
            hdr.ipv4.setValid();
            hdr.ipv4.version = 4w4;
            hdr.ipv4.ihl = 4w5;
            hdr.ipv4.dscp = 6w0;
            hdr.ipv4.ecn = 2w0;
            hdr.ipv4.total_len = hdr.ipv4.total_len + 16w44;
            hdr.ipv4.identification = 16w0x1513;
            hdr.ipv4.flags = 3w0;
            hdr.ipv4.frag_offset = 13w0;
            hdr.ipv4.ttl = 8w64;
            hdr.ipv4.proto = IpProtocol.UDP;
            hdr.ipv4.src_addr = local_meta.tunnel_out_src_ipv4_addr;
            hdr.ipv4.dst_addr = local_meta.tunnel_out_dst_ipv4_addr;
            hdr.ipv4.checksum = 16w0;
        }
        @hidden {
            hdr.gtpu.setValid();
            hdr.gtpu.version = 3w0x1;
            hdr.gtpu.pt = 1w0x1;
            hdr.gtpu.spare = 1w0;
            hdr.gtpu.seq_flag = 1w0;
            hdr.gtpu.npdu_flag = 1w0;
            hdr.gtpu.msgtype = GTPUMessageType.GPDU;
            hdr.gtpu.teid = local_meta.tunnel_out_teid;
        }
        hdr.gtpu.msglen = hdr.inner_ipv4.total_len + 16w8;
        hdr.gtpu.ex_flag = 1w1;
        hdr.gtpu_options.setValid();
        hdr.gtpu_options.seq_num = 16w0;
        hdr.gtpu_options.n_pdu_num = 8w0;
        hdr.gtpu_options.next_ext = 8w0x85;
        hdr.gtpu_ext_psc.setValid();
        hdr.gtpu_ext_psc.len = 8w1;
        hdr.gtpu_ext_psc.type = 4w0;
        hdr.gtpu_ext_psc.spare0 = 4w0;
        hdr.gtpu_ext_psc.ppp = 1w0;
        hdr.gtpu_ext_psc.rqi = 1w0;
        hdr.gtpu_ext_psc.qfi = local_meta.tunnel_out_qfi;
        hdr.gtpu_ext_psc.next_ext = 8w0x0;
    }
    @name("PreQosPipe.Routing.drop") action Routing_drop_0() {
        mark_to_drop(std_meta);
    }
    @name("PreQosPipe.Routing.route") action Routing_route_0(@name("src_mac") mac_addr_t src_mac, @name("dst_mac") mac_addr_t dst_mac, @name("egress_port") PortId_t egress_port_1) {
        std_meta.egress_spec = egress_port_1;
        hdr.ethernet.src_addr = src_mac;
        hdr.ethernet.dst_addr = dst_mac;
    }
    @name("PreQosPipe.Routing.routes_v4") table Routing_routes_v4 {
        key = {
            hdr.ipv4.dst_addr  : lpm @name("dst_prefix");
            hdr.ipv4.src_addr  : selector @name("hdr.ipv4.src_addr");
            hdr.ipv4.proto     : selector @name("hdr.ipv4.proto");
            local_meta.l4_sport: selector @name("local_meta.l4_sport");
            local_meta.l4_dport: selector @name("local_meta.l4_dport");
        }
        actions = {
            Routing_route_0();
            @defaultonly NoAction_3();
        }
        @name("hashed_selector") implementation = action_selector(HashAlgorithm.crc16, 32w1024, 32w16);
        size = 1024;
        default_action = NoAction_3();
    }
    @name("PreQosPipe.Acl.set_port") action Acl_set_port_0(@name("port") PortId_t port) {
        std_meta.egress_spec = port;
    }
    @name("PreQosPipe.Acl.punt") action Acl_punt_0() {
        std_meta.egress_spec = 9w255;
    }
    @name("PreQosPipe.Acl.clone_to_cpu") action Acl_clone_to_cpu_0() {
        clone_preserving_field_list(CloneType.I2E, 32w99, 8w0);
    }
    @name("PreQosPipe.Acl.drop") action Acl_drop_0() {
        mark_to_drop(std_meta);
        exit;
    }
    @name("PreQosPipe.Acl.acls") table Acl_acls {
        key = {
            std_meta.ingress_port  : ternary @name("inport");
            local_meta.src_iface   : ternary @name("src_iface");
            hdr.ethernet.src_addr  : ternary @name("eth_src");
            hdr.ethernet.dst_addr  : ternary @name("eth_dst");
            hdr.ethernet.ether_type: ternary @name("eth_type");
            hdr.ipv4.src_addr      : ternary @name("ipv4_src");
            hdr.ipv4.dst_addr      : ternary @name("ipv4_dst");
            hdr.ipv4.proto         : ternary @name("ipv4_proto");
            local_meta.l4_sport    : ternary @name("l4_sport");
            local_meta.l4_dport    : ternary @name("l4_dport");
        }
        actions = {
            Acl_set_port_0();
            Acl_punt_0();
            Acl_clone_to_cpu_0();
            Acl_drop_0();
            NoAction_4();
        }
        const default_action = NoAction_4();
        @name("acls") counters = direct_counter(CounterType.packets_and_bytes);
    }
    apply {
        hasReturned_0 = false;
        _initialize_metadata();
        if (hdr.packet_out.isValid()) {
            hdr.packet_out.setInvalid();
        } else {
            if (my_station_0.apply().hit) {
                ;
            } else {
                hasReturned_0 = true;
            }
            if (hasReturned_0) {
                ;
            } else if (interfaces_0.apply().hit) {
                if (local_meta.direction == Direction.UPLINK) {
                    local_meta.ue_addr = hdr.inner_ipv4.src_addr;
                    local_meta.inet_addr = hdr.inner_ipv4.dst_addr;
                    local_meta.ue_l4_port = local_meta.l4_sport;
                    local_meta.inet_l4_port = local_meta.l4_dport;
                    local_meta.ip_proto = hdr.inner_ipv4.proto;
                    sessions_uplink_0.apply();
                } else if (local_meta.direction == Direction.DOWNLINK) {
                    local_meta.ue_addr = hdr.ipv4.dst_addr;
                    local_meta.inet_addr = hdr.ipv4.src_addr;
                    local_meta.ue_l4_port = local_meta.l4_dport;
                    local_meta.inet_l4_port = local_meta.l4_sport;
                    local_meta.ip_proto = hdr.ipv4.proto;
                    sessions_downlink_0.apply();
                    tunnel_peers_0.apply();
                }
                applications_0.apply();
                if (local_meta.direction == Direction.UPLINK) {
                    terminations_uplink_0.apply();
                } else if (local_meta.direction == Direction.DOWNLINK) {
                    terminations_downlink_0.apply();
                }
                if (local_meta.terminations_hit) {
                    pre_qos_counter_0.count(local_meta.ctr_idx);
                }
                app_meter_0.execute_meter<MeterColor>(local_meta.app_meter_idx_internal, local_meta.app_color);
                if (local_meta.app_color == MeterColor.RED) {
                    local_meta.needs_dropping = true;
                } else {
                    session_meter_0.execute_meter<MeterColor>(local_meta.session_meter_idx_internal, local_meta.session_color);
                    if (local_meta.session_color == MeterColor.RED) {
                        local_meta.needs_dropping = true;
                    } else {
                        slice_tc_meter_0.execute_meter<MeterColor>(local_meta.slice_id ++ local_meta.tc, local_meta.slice_tc_color);
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
                    if (local_meta.tunnel_out_qfi == 6w0) {
                        do_gtpu_tunnel();
                    } else {
                        do_gtpu_tunnel_with_psc();
                    }
                }
                if (local_meta.needs_dropping) {
                    do_drop_4();
                }
            }
        }
        if (hasReturned_0) {
            ;
        } else {
            Routing_hasReturned = false;
            if (hdr.ipv4.isValid()) {
                ;
            } else {
                Routing_hasReturned = true;
            }
            if (Routing_hasReturned) {
                ;
            } else {
                hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
                if (hdr.ipv4.ttl == 8w0) {
                    Routing_drop_0();
                } else {
                    Routing_routes_v4.apply();
                }
            }
            if (hdr.ethernet.isValid() && hdr.ipv4.isValid()) {
                Acl_acls.apply();
            }
        }
    }
}

control PostQosPipe(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    @name("PostQosPipe.post_qos_counter") counter<counter_index_t>(32w1024, CounterType.packets_and_bytes) post_qos_counter_0;
    apply {
        post_qos_counter_0.count(local_meta.ctr_idx);
        if (std_meta.egress_port == 9w255) {
            hdr.packet_in.setValid();
            hdr.packet_in.ingress_port = local_meta.preserved_ingress_port;
            exit;
        }
    }
}

V1Switch<parsed_headers_t, local_metadata_t>(ParserImpl(), VerifyChecksumImpl(), PreQosPipe(), PostQosPipe(), ComputeChecksumImpl(), DeparserImpl()) main;
