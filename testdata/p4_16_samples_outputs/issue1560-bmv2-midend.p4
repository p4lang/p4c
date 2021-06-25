error {
    IPv4HeaderTooShort,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<32> IPv4Address;
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
    varbit<320> options;
}

header tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

header IPv4_up_to_ihl_only_h {
    bit<4> version;
    bit<4> ihl;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
}

struct mystruct1_t {
    bit<4> a;
    bit<4> b;
}

struct metadata {
    bit<4>  _mystruct1_a0;
    bit<4>  _mystruct1_b1;
    bit<16> _hash12;
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @name("parserI.tmp") IPv4_up_to_ihl_only_h tmp;
    bit<8> tmp_4;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        tmp_4 = pkt.lookahead<bit<8>>();
        tmp.setValid();
        tmp.version = tmp_4[7:4];
        tmp.ihl = tmp_4[3:0];
        pkt.extract<ipv4_t>(hdr.ipv4, (bit<32>)(((bit<9>)tmp_4[3:0] << 2) + 9w492 << 3));
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl >= 4w5, error.IPv4HeaderTooShort);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract<tcp_t>(hdr.tcp);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("cIngress.foo1") action foo1(@name("dstAddr") IPv4Address dstAddr_1) {
        hdr.ipv4.dstAddr = dstAddr_1;
    }
    @name("cIngress.foo1") action foo1_1(@name("dstAddr") IPv4Address dstAddr_2) {
        hdr.ipv4.dstAddr = dstAddr_2;
    }
    @name("cIngress.foo1") action foo1_2(@name("dstAddr") IPv4Address dstAddr_3) {
        hdr.ipv4.dstAddr = dstAddr_3;
    }
    @name("cIngress.foo2") action foo2(@name("srcAddr") IPv4Address srcAddr_1) {
        hdr.ipv4.srcAddr = srcAddr_1;
    }
    @name("cIngress.foo2") action foo2_1(@name("srcAddr") IPv4Address srcAddr_2) {
        hdr.ipv4.srcAddr = srcAddr_2;
    }
    @name("cIngress.foo2") action foo2_2(@name("srcAddr") IPv4Address srcAddr_3) {
        hdr.ipv4.srcAddr = srcAddr_3;
    }
    @name("cIngress.t0") table t0_0 {
        key = {
            hdr.tcp.dstPort: exact @name("hdr.tcp.dstPort") ;
        }
        actions = {
            foo1();
            foo2();
            @defaultonly NoAction_1();
        }
        size = 8;
        default_action = NoAction_1();
    }
    @name("cIngress.t1") table t1_0 {
        key = {
            hdr.tcp.dstPort: exact @name("hdr.tcp.dstPort") ;
        }
        actions = {
            foo1_1();
            foo2_1();
            @defaultonly NoAction_2();
        }
        size = 8;
        default_action = NoAction_2();
    }
    @name("cIngress.t2") table t2_0 {
        actions = {
            foo1_2();
            foo2_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.tcp.srcPort       : exact @name("hdr.tcp.srcPort") ;
            hdr.ipv4.dstAddr[15:0]: selector @name("meta.hash1") ;
        }
        size = 16;
        default_action = NoAction_3();
    }
    @hidden action issue1560bmv2l175() {
        meta._hash12 = hdr.ipv4.dstAddr[15:0];
    }
    @hidden table tbl_issue1560bmv2l175 {
        actions = {
            issue1560bmv2l175();
        }
        const default_action = issue1560bmv2l175();
    }
    apply {
        t0_0.apply();
        t1_0.apply();
        tbl_issue1560bmv2l175.apply();
        t2_0.apply();
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

