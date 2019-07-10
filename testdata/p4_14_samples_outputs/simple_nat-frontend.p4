#include <core.p4>
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
    @name(".meta") 
    meta_t meta;
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
    bit<64> tmp;
    @name(".parse_cpu_header") state parse_cpu_header {
        packet.extract<cpu_header_t>(hdr.cpu_header);
        meta.meta.if_index = hdr.cpu_header.if_index;
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
        meta.meta.ipv4_sa = hdr.ipv4.srcAddr;
        meta.meta.ipv4_da = hdr.ipv4.dstAddr;
        meta.meta.tcpLength = hdr.ipv4.totalLen + 16w65516;
        transition select(hdr.ipv4.protocol) {
            8w0x6: parse_tcp;
            default: accept;
        }
    }
    @name(".parse_tcp") state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        meta.meta.tcp_sp = hdr.tcp.srcPort;
        meta.meta.tcp_dp = hdr.tcp.dstPort;
        transition accept;
    }
    @name(".start") state start {
        meta.meta.if_index = (bit<8>)standard_metadata.ingress_port;
        tmp = packet.lookahead<bit<64>>();
        transition select(tmp[63:0]) {
            64w0: parse_cpu_header;
            default: parse_ethernet;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_1() {
    }
    @name(".do_rewrites") action do_rewrites(bit<48> smac) {
        hdr.cpu_header.setInvalid();
        hdr.ethernet.srcAddr = smac;
        hdr.ipv4.srcAddr = meta.meta.ipv4_sa;
        hdr.ipv4.dstAddr = meta.meta.ipv4_da;
        hdr.tcp.srcPort = meta.meta.tcp_sp;
        hdr.tcp.dstPort = meta.meta.tcp_dp;
    }
    @name("._drop") action _drop() {
        mark_to_drop(standard_metadata);
    }
    @name(".do_cpu_encap") action do_cpu_encap() {
        hdr.cpu_header.setValid();
        hdr.cpu_header.preamble = 64w0;
        hdr.cpu_header.device = 8w0;
        hdr.cpu_header.reason = 8w0xab;
        hdr.cpu_header.if_index = meta.meta.if_index;
    }
    @name(".send_frame") table send_frame_0 {
        actions = {
            do_rewrites();
            _drop();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.egress_port: exact @name("standard_metadata.egress_port") ;
        }
        size = 256;
        default_action = NoAction_0();
    }
    @name(".send_to_cpu") table send_to_cpu_0 {
        actions = {
            do_cpu_encap();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
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
    @name(".NoAction") action NoAction_8() {
    }
    @name(".NoAction") action NoAction_9() {
    }
    @name(".NoAction") action NoAction_10() {
    }
    @name(".NoAction") action NoAction_11() {
    }
    @name(".set_dmac") action set_dmac(bit<48> dmac) {
        hdr.ethernet.dstAddr = dmac;
    }
    @name("._drop") action _drop_2() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_6() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_7() {
        mark_to_drop(standard_metadata);
    }
    @name("._drop") action _drop_8() {
        mark_to_drop(standard_metadata);
    }
    @name(".set_if_info") action set_if_info(bit<32> ipv4_addr, bit<48> mac_addr, bit<1> is_ext) {
        meta.meta.if_ipv4_addr = ipv4_addr;
        meta.meta.if_mac_addr = mac_addr;
        meta.meta.is_ext_if = is_ext;
    }
    @name(".set_nhop") action set_nhop(bit<32> nhop_ipv4, bit<9> port) {
        meta.meta.nhop_ipv4 = nhop_ipv4;
        standard_metadata.egress_spec = port;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name(".nat_miss_int_to_ext") action nat_miss_int_to_ext() {
        clone3<tuple<standard_metadata_t>>(CloneType.I2E, 32w250, { standard_metadata });
    }
    @name(".nat_miss_ext_to_int") action nat_miss_ext_to_int() {
        meta.meta.do_forward = 1w0;
        mark_to_drop(standard_metadata);
    }
    @name(".nat_hit_int_to_ext") action nat_hit_int_to_ext(bit<32> srcAddr, bit<16> srcPort) {
        meta.meta.do_forward = 1w1;
        meta.meta.ipv4_sa = srcAddr;
        meta.meta.tcp_sp = srcPort;
    }
    @name(".nat_hit_ext_to_int") action nat_hit_ext_to_int(bit<32> dstAddr, bit<16> dstPort) {
        meta.meta.do_forward = 1w1;
        meta.meta.ipv4_da = dstAddr;
        meta.meta.tcp_dp = dstPort;
    }
    @name(".nat_no_nat") action nat_no_nat() {
        meta.meta.do_forward = 1w1;
    }
    @name(".forward") table forward_0 {
        actions = {
            set_dmac();
            _drop_2();
            @defaultonly NoAction_8();
        }
        key = {
            meta.meta.nhop_ipv4: exact @name("meta.nhop_ipv4") ;
        }
        size = 512;
        default_action = NoAction_8();
    }
    @name(".if_info") table if_info_0 {
        actions = {
            _drop_6();
            set_if_info();
            @defaultonly NoAction_9();
        }
        key = {
            meta.meta.if_index: exact @name("meta.if_index") ;
        }
        default_action = NoAction_9();
    }
    @name(".ipv4_lpm") table ipv4_lpm_0 {
        actions = {
            set_nhop();
            _drop_7();
            @defaultonly NoAction_10();
        }
        key = {
            meta.meta.ipv4_da: lpm @name("meta.ipv4_da") ;
        }
        size = 1024;
        default_action = NoAction_10();
    }
    @name(".nat") table nat_0 {
        actions = {
            _drop_8();
            nat_miss_int_to_ext();
            nat_miss_ext_to_int();
            nat_hit_int_to_ext();
            nat_hit_ext_to_int();
            nat_no_nat();
            @defaultonly NoAction_11();
        }
        key = {
            meta.meta.is_ext_if: exact @name("meta.is_ext_if") ;
            hdr.ipv4.isValid() : exact @name("ipv4.$valid$") ;
            hdr.tcp.isValid()  : exact @name("tcp.$valid$") ;
            hdr.ipv4.srcAddr   : ternary @name("ipv4.srcAddr") ;
            hdr.ipv4.dstAddr   : ternary @name("ipv4.dstAddr") ;
            hdr.tcp.srcPort    : ternary @name("tcp.srcPort") ;
            hdr.tcp.dstPort    : ternary @name("tcp.dstPort") ;
        }
        size = 128;
        default_action = NoAction_11();
    }
    apply {
        if_info_0.apply();
        nat_0.apply();
        if (meta.meta.do_forward == 1w1 && hdr.ipv4.ttl > 8w0) {
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

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<32>, bit<32>, bit<4>, bit<4>, bit<8>, bit<16>, bit<16>>, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.meta.tcpLength, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr }, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<32>, bit<32>, bit<4>, bit<4>, bit<8>, bit<16>, bit<16>>, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.meta.tcpLength, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.flags, hdr.tcp.window, hdr.tcp.urgentPtr }, hdr.tcp.checksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

