#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4> mcast_grp;
    bit<4> egress_rid;
}

struct meta_t {
    bit<1>  do_forward;
    bit<32> ipv4_sa;
    bit<32> ipv4_da;
    bit<16> tcp_sp;
    bit<16> tcp_dp;
    bit<32> nhop_ipv4;
    bit<32> if_ipv4_addr;
    bit<48> if_mac_addr;
    bit<1>  is_ext_if;
    bit<16> tcpLength;
    bit<8>  if_index;
}

header cpu_header_t {
    bit<64> preamble;
    bit<8>  device;
    bit<8>  reason;
    bit<8>  if_index;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  res;
    bit<8>  flags;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct metadata {
    bit<1>  _meta_do_forward0;
    bit<32> _meta_ipv4_sa1;
    bit<32> _meta_ipv4_da2;
    bit<16> _meta_tcp_sp3;
    bit<16> _meta_tcp_dp4;
    bit<32> _meta_nhop_ipv45;
    bit<32> _meta_if_ipv4_addr6;
    bit<48> _meta_if_mac_addr7;
    bit<1>  _meta_is_ext_if8;
    bit<16> _meta_tcpLength9;
    bit<8>  _meta_if_index10;
}

struct headers {
    @name(".cpu_header")
    cpu_header_t cpu_header;
    @name(".ethernet")
    ethernet_t   ethernet;
    @name(".ipv4")
    ipv4_t       ipv4;
    @name(".tcp")
    tcp_t        tcp;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.tmp_0") bit<64> tmp_0;
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        meta._meta_if_index10 = hdr.cpu_header.if_index;
        transition parse_ethernet;
    }
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    @name(".parse_ipv4") state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        meta._meta_ipv4_sa1 = hdr.ipv4.srcAddr;
        meta._meta_ipv4_da2 = hdr.ipv4.dstAddr;
        meta._meta_tcpLength9 = hdr.ipv4.totalLen + 16w65516;
        transition select(hdr.ipv4.protocol) {
            8w0x6: parse_tcp;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        meta._meta_tcp_sp3 = hdr.tcp.srcPort;
        meta._meta_tcp_dp4 = hdr.tcp.dstPort;
        transition accept;
    }
    @name(".start") state start {
        meta._meta_if_index10 = (bit<8>)standard_metadata.ingress_port;
        tmp_0 = packet.lookahead<bit<64>>();
        transition select(tmp_0) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name(".do_rewrites") action do_rewrites(@name("smac") bit<48> smac) {
        hdr.cpu_header.setInvalid();
        hdr.ethernet.srcAddr = smac;
        hdr.ipv4.srcAddr = meta._meta_ipv4_sa1;
        hdr.ipv4.dstAddr = meta._meta_ipv4_da2;
        hdr.tcp.srcPort = meta._meta_tcp_sp3;
        hdr.tcp.dstPort = meta._meta_tcp_dp4;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".do_cpu_encap") action do_cpu_encap() {
        hdr.cpu_header.setValid();
        hdr.cpu_header.preamble = 64w0;
        hdr.cpu_header.device = 8w0;
        hdr.cpu_header.reason = 8w0xab;
        hdr.cpu_header.if_index = meta._meta_if_index10;
    }
    @name(".send_frame") table send_frame_0 {
        actions = {
            do_rewrites();
            _drop();
            @defaultonly NoAction_2();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port");
        }
        size = 256;
        default_action = NoAction_2();
    }
    @name(".send_to_cpu") table send_to_cpu_0 {
        actions = {
            do_cpu_encap();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    apply {
        if (standard_metadata.instance_type == 32w0) {
            send_frame_0.apply();
        } else {
            send_to_cpu_0.apply();
        }
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_5() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_6() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_7() {
    }
    @name(".set_dmac") action set_dmac(@name("dmac") bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("._drop") action _drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_3() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_4() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_5() {
        mark_to_drop(standard_metadata);
    }
    @name(".set_if_info") action set_if_info(@name("ipv4_addr") bit<32> ipv4_addr, @name("mac_addr") bit<48> mac_addr, @name("is_ext") bit<1> is_ext) {
        meta._meta_if_ipv4_addr6 = ipv4_addr;
        meta._meta_if_mac_addr7 = mac_addr;
        meta._meta_is_ext_if8 = is_ext;
    }
    @name(".set_nhop") action set_nhop(@name("nhop_ipv4") bit<32> nhop_ipv4_1, @name("port") bit<9> port) {
        meta._meta_nhop_ipv45 = nhop_ipv4_1;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".nat_miss_int_to_ext") action nat_miss_int_to_ext() {
        clone_preserving_field_list(CloneType.I2E, 32w250, 8w1);
    }
    @name(".nat_miss_ext_to_int") action nat_miss_ext_to_int() {
        meta._meta_do_forward0 = 1w0;
        mark_to_drop(standard_metadata);
    }
    @name(".nat_hit_int_to_ext") action nat_hit_int_to_ext(@name("srcAddr") bit<32> srcAddr_1, @name("srcPort") bit<16> srcPort_1) {
        meta._meta_do_forward0 = 1w1;
        meta._meta_ipv4_sa1 = srcAddr_1;
        meta._meta_tcp_sp3 = srcPort_1;
    }
    @name(".nat_hit_ext_to_int") action nat_hit_ext_to_int(@name("dstAddr") bit<32> dstAddr_1, @name("dstPort") bit<16> dstPort_1) {
        meta._meta_do_forward0 = 1w1;
        meta._meta_ipv4_da2 = dstAddr_1;
        meta._meta_tcp_dp4 = dstPort_1;
    }
    @name(".nat_no_nat") action nat_no_nat() {
        meta._meta_do_forward0 = 1w1;
    }
    @name(".forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_2();
            @defaultonly NoAction_4();
        }
        key = {
            meta._meta_nhop_ipv45: exact @name("meta.nhop_ipv4");
        }
        size = 512;
        default_action = NoAction_4();
    }
    @name(".if_info") table if_info_0 {
        actions = {
            _drop_3();
            set_if_info();
            @defaultonly NoAction_5();
        }
        key = {
            meta._meta_if_index10: exact @name("meta.if_index");
        }
        default_action = NoAction_5();
    }
    @name(".ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_4();
            @defaultonly NoAction_6();
        }
        key = {
            meta._meta_ipv4_da2: lpm @name("meta.ipv4_da");
        }
        size = 1024;
        default_action = NoAction_6();
    }
    @name(".nat") table nat_0 {
        actions = {
            _drop_5();
            nat_miss_int_to_ext();
            nat_miss_ext_to_int();
            nat_hit_int_to_ext();
            nat_hit_ext_to_int();
            nat_no_nat();
            @defaultonly NoAction_7();
        }
        key = {
            meta._meta_is_ext_if8: exact @name("meta.is_ext_if");
            hdr.ipv4.isValid()   : exact @name("ipv4.$valid$");
            hdr.tcp.isValid()    : exact @name("tcp.$valid$");
            hdr.ipv4.srcAddr     : ternary @name("ipv4.srcAddr");
            hdr.ipv4.dstAddr     : ternary @name("ipv4.dstAddr");
            hdr.tcp.srcPort      : ternary @name("tcp.srcPort");
            hdr.tcp.dstPort      : ternary @name("tcp.dstPort");
        }
        size = 128;
        default_action = NoAction_7();
    }
    apply {
        if_info_0.apply();
        nat_0.apply();
        if (meta._meta_do_forward0 == 1w1 && hdr.ipv4.ttl > 8w0) {
            ipv4_lpm_0.apply();
            forward_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<cpu_header_t>(hdr.cpu_header);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

struct tuple_0 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

struct tuple_1 {
    bit<32> f0;
    bit<32> f1;
    bit<8>  f2;
    bit<8>  f3;
    bit<16> f4;
    bit<16> f5;
    bit<16> f6;
    bit<32> f7;
    bit<32> f8;
    bit<4>  f9;
    bit<4>  f10;
    bit<8>  f11;
    bit<16> f12;
    bit<16> f13;
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(true, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple_1, bit<16>>(hdr.tcp.isValid(), (tuple_1){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = 8w0,f3 = hdr.ipv4.protocol,f4 = meta._meta_tcpLength9,f5 = hdr.tcp.srcPort,f6 = hdr.tcp.dstPort,f7 = hdr.tcp.seqNo,f8 = hdr.tcp.ackNo,f9 = hdr.tcp.dataOffset,f10 = hdr.tcp.res,f11 = hdr.tcp.flags,f12 = hdr.tcp.window,f13 = hdr.tcp.urgentPtr}, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(true, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple_1, bit<16>>(hdr.tcp.isValid(), (tuple_1){f0 = hdr.ipv4.srcAddr,f1 = hdr.ipv4.dstAddr,f2 = 8w0,f3 = hdr.ipv4.protocol,f4 = meta._meta_tcpLength9,f5 = hdr.tcp.srcPort,f6 = hdr.tcp.dstPort,f7 = hdr.tcp.seqNo,f8 = hdr.tcp.ackNo,f9 = hdr.tcp.dataOffset,f10 = hdr.tcp.res,f11 = hdr.tcp.flags,f12 = hdr.tcp.window,f13 = hdr.tcp.urgentPtr}, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
