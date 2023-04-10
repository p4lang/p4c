#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<6>  dscp;
    bit<2>  ecn;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  proto;
    bit<16> checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

header tcp_t {
    bit<16> sport;
    bit<16> dport;
    bit<32> seq_no;
    bit<32> ack_no;
    bit<4>  data_offset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgent_ptr;
}

header udp_t {
    bit<16> sport;
    bit<16> dport;
    bit<16> len;
    bit<16> checksum;
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
    bit<32> teid;
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
    bit<9> ingress_port;
    bit<7> _pad;
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
    bit<32> ue_address;
}

struct local_metadata_t {
    bit<8>  direction;
    bit<32> teid;
    bit<4>  slice_id;
    bit<2>  tc;
    bit<32> next_hop_ip;
    bit<32> ue_addr;
    bit<32> inet_addr;
    bit<16> ue_l4_port;
    bit<16> inet_l4_port;
    bit<16> l4_sport;
    bit<16> l4_dport;
    bit<8>  ip_proto;
    bit<8>  application_id;
    bit<8>  src_iface;
    bool    needs_gtpu_decap;
    bool    needs_tunneling;
    bool    needs_buffering;
    bool    needs_dropping;
    bool    terminations_hit;
    bit<32> ctr_idx;
    bit<8>  tunnel_peer_id;
    bit<32> tunnel_out_src_ipv4_addr;
    bit<32> tunnel_out_dst_ipv4_addr;
    bit<16> tunnel_out_udp_sport;
    bit<32> tunnel_out_teid;
    bit<6>  tunnel_out_qfi;
    bit<32> session_meter_idx_internal;
    bit<32> app_meter_idx_internal;
    bit<2>  session_color;
    bit<2>  app_color;
    bit<2>  slice_tc_color;
    @field_list(0)
    bit<9>  preserved_ingress_port;
}

parser ParserImpl(packet_in packet, out parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    @name("ParserImpl.gtpu") gtpu_t gtpu_0;
    @name("ParserImpl.gtpu_ext_len") bit<8> gtpu_ext_len_0;
    bit<64> tmp;
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
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.proto) {
            8w17: parse_udp;
            8w6: parse_tcp;
            8w1: parse_icmp;
            default: accept;
        }
    }
    state parse_udp {
        packet.extract<udp_t>(hdr.udp);
        local_meta.l4_sport = hdr.udp.sport;
        local_meta.l4_dport = hdr.udp.dport;
        tmp = packet.lookahead<bit<64>>();
        gtpu_0.setValid();
        gtpu_0.version = tmp[63:61];
        gtpu_0.pt = tmp[60:60];
        gtpu_0.spare = tmp[59:59];
        gtpu_0.ex_flag = tmp[58:58];
        gtpu_0.seq_flag = tmp[57:57];
        gtpu_0.npdu_flag = tmp[56:56];
        gtpu_0.msgtype = tmp[55:48];
        gtpu_0.msglen = tmp[47:32];
        gtpu_0.teid = tmp[31:0];
        transition select(hdr.udp.dport, tmp[63:61], tmp[55:48]) {
            (16w9875, default, default): parse_inner_ipv4;
            (16w2152, 3w0x1, 8w255): parse_gtpu;
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
            8w17: parse_inner_udp;
            8w6: parse_inner_tcp;
            8w1: parse_inner_icmp;
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

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<6>  f2;
    bit<2>  f3;
    bit<16> f4;
    bit<16> f5;
    bit<3>  f6;
    bit<13> f7;
    bit<8>  f8;
    bit<8>  f9;
    bit<32> f10;
    bit<32> f11;
}

control VerifyChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.dscp,f3 = hdr.ipv4.ecn,f4 = hdr.ipv4.total_len,f5 = hdr.ipv4.identification,f6 = hdr.ipv4.flags,f7 = hdr.ipv4.frag_offset,f8 = hdr.ipv4.ttl,f9 = hdr.ipv4.proto,f10 = hdr.ipv4.src_addr,f11 = hdr.ipv4.dst_addr}, hdr.ipv4.checksum, HashAlgorithm.csum16);
        verify_checksum<tuple_0, bit<16>>(hdr.inner_ipv4.isValid(), (tuple_0){f0 = hdr.inner_ipv4.version,f1 = hdr.inner_ipv4.ihl,f2 = hdr.inner_ipv4.dscp,f3 = hdr.inner_ipv4.ecn,f4 = hdr.inner_ipv4.total_len,f5 = hdr.inner_ipv4.identification,f6 = hdr.inner_ipv4.flags,f7 = hdr.inner_ipv4.frag_offset,f8 = hdr.inner_ipv4.ttl,f9 = hdr.inner_ipv4.proto,f10 = hdr.inner_ipv4.src_addr,f11 = hdr.inner_ipv4.dst_addr}, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control ComputeChecksumImpl(inout parsed_headers_t hdr, inout local_metadata_t local_meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.dscp,f3 = hdr.ipv4.ecn,f4 = hdr.ipv4.total_len,f5 = hdr.ipv4.identification,f6 = hdr.ipv4.flags,f7 = hdr.ipv4.frag_offset,f8 = hdr.ipv4.ttl,f9 = hdr.ipv4.proto,f10 = hdr.ipv4.src_addr,f11 = hdr.ipv4.dst_addr}, hdr.ipv4.checksum, HashAlgorithm.csum16);
        update_checksum<tuple_0, bit<16>>(hdr.inner_ipv4.isValid(), (tuple_0){f0 = hdr.inner_ipv4.version,f1 = hdr.inner_ipv4.ihl,f2 = hdr.inner_ipv4.dscp,f3 = hdr.inner_ipv4.ecn,f4 = hdr.inner_ipv4.total_len,f5 = hdr.inner_ipv4.identification,f6 = hdr.inner_ipv4.flags,f7 = hdr.inner_ipv4.frag_offset,f8 = hdr.inner_ipv4.ttl,f9 = hdr.inner_ipv4.proto,f10 = hdr.inner_ipv4.src_addr,f11 = hdr.inner_ipv4.dst_addr}, hdr.inner_ipv4.checksum, HashAlgorithm.csum16);
    }
}

control PreQosPipe(inout parsed_headers_t hdr, inout local_metadata_t local_meta, inout standard_metadata_t std_meta) {
    bool hasExited;
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
    @name("PreQosPipe.pre_qos_counter") counter<bit<32>>(32w1024, CounterType.packets_and_bytes) pre_qos_counter_0;
    @name("PreQosPipe.app_meter") meter<bit<32>>(32w1024, MeterType.bytes) app_meter_0;
    @name("PreQosPipe.session_meter") meter<bit<32>>(32w1024, MeterType.bytes) session_meter_0;
    @name("PreQosPipe.slice_tc_meter") meter<bit<6>>(32w64, MeterType.bytes) slice_tc_meter_0;
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
    @name("PreQosPipe.set_source_iface") action set_source_iface(@name("src_iface") bit<8> src_iface_1, @name("direction") bit<8> direction_1, @name("slice_id") bit<4> slice_id_1) {
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
        const default_action = set_source_iface(8w0x0, 8w0x0, 4w0x0);
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
        hasExited = true;
    }
    @name("PreQosPipe.do_drop") action do_drop() {
        mark_to_drop(std_meta);
        hasExited = true;
    }
    @name("PreQosPipe.do_drop") action do_drop_1() {
        mark_to_drop(std_meta);
        hasExited = true;
    }
    @name("PreQosPipe.do_drop") action do_drop_2() {
        mark_to_drop(std_meta);
        hasExited = true;
    }
    @name("PreQosPipe.do_drop") action do_drop_3() {
        mark_to_drop(std_meta);
        hasExited = true;
    }
    @name("PreQosPipe.do_drop") action do_drop_4() {
        mark_to_drop(std_meta);
        hasExited = true;
    }
    @name("PreQosPipe.set_session_uplink") action set_session_uplink(@name("session_meter_idx") bit<32> session_meter_idx) {
        local_meta.needs_gtpu_decap = true;
        local_meta.session_meter_idx_internal = session_meter_idx;
    }
    @name("PreQosPipe.set_session_uplink_drop") action set_session_uplink_drop() {
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.set_session_downlink") action set_session_downlink(@name("tunnel_peer_id") bit<8> tunnel_peer_id_1, @name("session_meter_idx") bit<32> session_meter_idx_3) {
        local_meta.tunnel_peer_id = tunnel_peer_id_1;
        local_meta.session_meter_idx_internal = session_meter_idx_3;
    }
    @name("PreQosPipe.set_session_downlink_drop") action set_session_downlink_drop() {
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.set_session_downlink_buff") action set_session_downlink_buff(@name("session_meter_idx") bit<32> session_meter_idx_4) {
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
    @name("PreQosPipe.uplink_term_fwd") action uplink_term_fwd(@name("ctr_idx") bit<32> ctr_idx_0, @name("tc") bit<2> tc_2, @name("app_meter_idx") bit<32> app_meter_idx) {
        local_meta.ctr_idx = ctr_idx_0;
        local_meta.terminations_hit = true;
        local_meta.app_meter_idx_internal = app_meter_idx;
        local_meta.tc = tc_2;
    }
    @name("PreQosPipe.uplink_term_drop") action uplink_term_drop(@name("ctr_idx") bit<32> ctr_idx_5) {
        local_meta.ctr_idx = ctr_idx_5;
        local_meta.terminations_hit = true;
        local_meta.needs_dropping = true;
    }
    @name("PreQosPipe.downlink_term_fwd") action downlink_term_fwd(@name("ctr_idx") bit<32> ctr_idx_6, @name("teid") bit<32> teid_1, @name("qfi") bit<6> qfi_1, @name("tc") bit<2> tc_3, @name("app_meter_idx") bit<32> app_meter_idx_2) {
        local_meta.ctr_idx = ctr_idx_6;
        local_meta.terminations_hit = true;
        local_meta.tunnel_out_teid = teid_1;
        local_meta.tunnel_out_qfi = qfi_1;
        local_meta.app_meter_idx_internal = app_meter_idx_2;
        local_meta.tc = tc_3;
    }
    @name("PreQosPipe.downlink_term_drop") action downlink_term_drop(@name("ctr_idx") bit<32> ctr_idx_7) {
        local_meta.ctr_idx = ctr_idx_7;
        local_meta.terminations_hit = true;
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
    @name("PreQosPipe.load_tunnel_param") action load_tunnel_param(@name("src_addr") bit<32> src_addr_1, @name("dst_addr") bit<32> dst_addr_1, @name("sport") bit<16> sport_1) {
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
        hdr.inner_udp = hdr.udp;
        hdr.udp.setInvalid();
        hdr.inner_tcp = hdr.tcp;
        hdr.tcp.setInvalid();
        hdr.inner_icmp = hdr.icmp;
        hdr.icmp.setInvalid();
        hdr.udp.setValid();
        hdr.udp.sport = local_meta.tunnel_out_udp_sport;
        hdr.udp.dport = 16w2152;
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
        hdr.ipv4.proto = 8w17;
        hdr.ipv4.src_addr = local_meta.tunnel_out_src_ipv4_addr;
        hdr.ipv4.dst_addr = local_meta.tunnel_out_dst_ipv4_addr;
        hdr.ipv4.checksum = 16w0;
        hdr.gtpu.setValid();
        hdr.gtpu.version = 3w0x1;
        hdr.gtpu.pt = 1w0x1;
        hdr.gtpu.spare = 1w0;
        hdr.gtpu.ex_flag = 1w0;
        hdr.gtpu.seq_flag = 1w0;
        hdr.gtpu.npdu_flag = 1w0;
        hdr.gtpu.msgtype = 8w255;
        hdr.gtpu.msglen = hdr.inner_ipv4.total_len;
        hdr.gtpu.teid = local_meta.tunnel_out_teid;
    }
    @name("PreQosPipe.do_gtpu_tunnel_with_psc") action do_gtpu_tunnel_with_psc() {
        hdr.inner_udp = hdr.udp;
        hdr.udp.setInvalid();
        hdr.inner_tcp = hdr.tcp;
        hdr.tcp.setInvalid();
        hdr.inner_icmp = hdr.icmp;
        hdr.icmp.setInvalid();
        hdr.udp.setValid();
        hdr.udp.sport = local_meta.tunnel_out_udp_sport;
        hdr.udp.dport = 16w2152;
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
        hdr.ipv4.proto = 8w17;
        hdr.ipv4.src_addr = local_meta.tunnel_out_src_ipv4_addr;
        hdr.ipv4.dst_addr = local_meta.tunnel_out_dst_ipv4_addr;
        hdr.ipv4.checksum = 16w0;
        hdr.gtpu.setValid();
        hdr.gtpu.version = 3w0x1;
        hdr.gtpu.pt = 1w0x1;
        hdr.gtpu.spare = 1w0;
        hdr.gtpu.seq_flag = 1w0;
        hdr.gtpu.npdu_flag = 1w0;
        hdr.gtpu.msgtype = 8w255;
        hdr.gtpu.teid = local_meta.tunnel_out_teid;
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
    @name("PreQosPipe.Routing.route") action Routing_route_0(@name("src_mac") bit<48> src_mac, @name("dst_mac") bit<48> dst_mac, @name("egress_port") bit<9> egress_port_1) {
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
    @name("PreQosPipe.Acl.set_port") action Acl_set_port_0(@name("port") bit<9> port) {
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
        hasExited = true;
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
    @hidden action act() {
        hasExited = false;
        hasReturned_0 = false;
    }
    @hidden action up4l681() {
        hdr.packet_out.setInvalid();
    }
    @hidden action up4l684() {
        hasReturned_0 = true;
    }
    @hidden action up4l688() {
        local_meta.ue_addr = hdr.inner_ipv4.src_addr;
        local_meta.inet_addr = hdr.inner_ipv4.dst_addr;
        local_meta.ue_l4_port = local_meta.l4_sport;
        local_meta.inet_l4_port = local_meta.l4_dport;
        local_meta.ip_proto = hdr.inner_ipv4.proto;
    }
    @hidden action up4l695() {
        local_meta.ue_addr = hdr.ipv4.dst_addr;
        local_meta.inet_addr = hdr.ipv4.src_addr;
        local_meta.ue_l4_port = local_meta.l4_dport;
        local_meta.inet_l4_port = local_meta.l4_sport;
        local_meta.ip_proto = hdr.ipv4.proto;
    }
    @hidden action up4l710() {
        pre_qos_counter_0.count(local_meta.ctr_idx);
    }
    @hidden action up4l714() {
        local_meta.needs_dropping = true;
    }
    @hidden action up4l718() {
        local_meta.needs_dropping = true;
    }
    @hidden action up4l722() {
        local_meta.needs_dropping = true;
    }
    @hidden action up4l720() {
        slice_tc_meter_0.execute_meter<bit<2>>(local_meta.slice_id ++ local_meta.tc, local_meta.slice_tc_color);
    }
    @hidden action up4l716() {
        session_meter_0.execute_meter<bit<2>>(local_meta.session_meter_idx_internal, local_meta.session_color);
    }
    @hidden action up4l712() {
        app_meter_0.execute_meter<bit<2>>(local_meta.app_meter_idx_internal, local_meta.app_color);
    }
    @hidden action up4l431() {
        Routing_hasReturned = true;
    }
    @hidden action act_0() {
        Routing_hasReturned = false;
    }
    @hidden action up4l433() {
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl__initialize_metadata {
        actions = {
            _initialize_metadata();
        }
        const default_action = _initialize_metadata();
    }
    @hidden table tbl_up4l681 {
        actions = {
            up4l681();
        }
        const default_action = up4l681();
    }
    @hidden table tbl_up4l684 {
        actions = {
            up4l684();
        }
        const default_action = up4l684();
    }
    @hidden table tbl_up4l688 {
        actions = {
            up4l688();
        }
        const default_action = up4l688();
    }
    @hidden table tbl_up4l695 {
        actions = {
            up4l695();
        }
        const default_action = up4l695();
    }
    @hidden table tbl_up4l710 {
        actions = {
            up4l710();
        }
        const default_action = up4l710();
    }
    @hidden table tbl_up4l712 {
        actions = {
            up4l712();
        }
        const default_action = up4l712();
    }
    @hidden table tbl_up4l714 {
        actions = {
            up4l714();
        }
        const default_action = up4l714();
    }
    @hidden table tbl_up4l716 {
        actions = {
            up4l716();
        }
        const default_action = up4l716();
    }
    @hidden table tbl_up4l718 {
        actions = {
            up4l718();
        }
        const default_action = up4l718();
    }
    @hidden table tbl_up4l720 {
        actions = {
            up4l720();
        }
        const default_action = up4l720();
    }
    @hidden table tbl_up4l722 {
        actions = {
            up4l722();
        }
        const default_action = up4l722();
    }
    @hidden table tbl_gtpu_decap {
        actions = {
            gtpu_decap();
        }
        const default_action = gtpu_decap();
    }
    @hidden table tbl_do_buffer {
        actions = {
            do_buffer();
        }
        const default_action = do_buffer();
    }
    @hidden table tbl_do_gtpu_tunnel {
        actions = {
            do_gtpu_tunnel();
        }
        const default_action = do_gtpu_tunnel();
    }
    @hidden table tbl_do_gtpu_tunnel_with_psc {
        actions = {
            do_gtpu_tunnel_with_psc();
        }
        const default_action = do_gtpu_tunnel_with_psc();
    }
    @hidden table tbl_do_drop {
        actions = {
            do_drop_4();
        }
        const default_action = do_drop_4();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_up4l431 {
        actions = {
            up4l431();
        }
        const default_action = up4l431();
    }
    @hidden table tbl_up4l433 {
        actions = {
            up4l433();
        }
        const default_action = up4l433();
    }
    @hidden table tbl_Routing_drop {
        actions = {
            Routing_drop_0();
        }
        const default_action = Routing_drop_0();
    }
    apply {
        tbl_act.apply();
        tbl__initialize_metadata.apply();
        if (hdr.packet_out.isValid()) {
            tbl_up4l681.apply();
        } else {
            if (my_station_0.apply().hit) {
                ;
            } else {
                tbl_up4l684.apply();
            }
            if (hasReturned_0) {
                ;
            } else if (interfaces_0.apply().hit) {
                if (local_meta.direction == 8w0x1) {
                    tbl_up4l688.apply();
                    sessions_uplink_0.apply();
                } else if (local_meta.direction == 8w0x2) {
                    tbl_up4l695.apply();
                    sessions_downlink_0.apply();
                    if (hasExited) {
                        ;
                    } else {
                        tunnel_peers_0.apply();
                    }
                }
                if (hasExited) {
                    ;
                } else {
                    applications_0.apply();
                    if (local_meta.direction == 8w0x1) {
                        terminations_uplink_0.apply();
                    } else if (local_meta.direction == 8w0x2) {
                        terminations_downlink_0.apply();
                    }
                }
                if (hasExited) {
                    ;
                } else {
                    if (local_meta.terminations_hit) {
                        tbl_up4l710.apply();
                    }
                    tbl_up4l712.apply();
                    if (local_meta.app_color == 2w2) {
                        tbl_up4l714.apply();
                    } else {
                        tbl_up4l716.apply();
                        if (local_meta.session_color == 2w2) {
                            tbl_up4l718.apply();
                        } else {
                            tbl_up4l720.apply();
                            if (local_meta.slice_tc_color == 2w2) {
                                tbl_up4l722.apply();
                            }
                        }
                    }
                    if (local_meta.needs_gtpu_decap) {
                        tbl_gtpu_decap.apply();
                    }
                    if (local_meta.needs_buffering) {
                        tbl_do_buffer.apply();
                    }
                }
                if (hasExited) {
                    ;
                } else {
                    if (local_meta.needs_tunneling) {
                        if (local_meta.tunnel_out_qfi == 6w0) {
                            tbl_do_gtpu_tunnel.apply();
                        } else {
                            tbl_do_gtpu_tunnel_with_psc.apply();
                        }
                    }
                    if (local_meta.needs_dropping) {
                        tbl_do_drop.apply();
                    }
                }
            }
        }
        if (hasExited) {
            ;
        } else if (hasReturned_0) {
            ;
        } else {
            tbl_act_0.apply();
            if (hdr.ipv4.isValid()) {
                ;
            } else {
                tbl_up4l431.apply();
            }
            if (Routing_hasReturned) {
                ;
            } else {
                tbl_up4l433.apply();
                if (hdr.ipv4.ttl == 8w0) {
                    tbl_Routing_drop.apply();
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
    bool hasExited_0;
    @name("PostQosPipe.post_qos_counter") counter<bit<32>>(32w1024, CounterType.packets_and_bytes) post_qos_counter_0;
    @hidden action up4l754() {
        hdr.packet_in.setValid();
        hdr.packet_in.ingress_port = local_meta.preserved_ingress_port;
        hasExited_0 = true;
    }
    @hidden action up4l752() {
        hasExited_0 = false;
        post_qos_counter_0.count(local_meta.ctr_idx);
    }
    @hidden table tbl_up4l752 {
        actions = {
            up4l752();
        }
        const default_action = up4l752();
    }
    @hidden table tbl_up4l754 {
        actions = {
            up4l754();
        }
        const default_action = up4l754();
    }
    apply {
        tbl_up4l752.apply();
        if (std_meta.egress_port == 9w255) {
            tbl_up4l754.apply();
        }
    }
}

V1Switch<parsed_headers_t, local_metadata_t>(ParserImpl(), VerifyChecksumImpl(), PreQosPipe(), PostQosPipe(), ComputeChecksumImpl(), DeparserImpl()) main;
