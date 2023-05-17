error {
    IPv4HeaderTooShort,
    IPv4IncorrectVersion
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

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
    bit<3>  res;
    bit<3>  ecn;
    bit<6>  ctrl;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
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
    bit<4> _mystruct1_a0;
    bit<4> _mystruct1_b1;
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4);
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
    @hidden action checksum2bmv2l130() {
        hdr.ethernet.srcAddr = 48w0xbad;
    }
    @hidden action checksum2bmv2l126() {
        stdmeta.egress_spec = 9w0;
    }
    @hidden action checksum2bmv2l134() {
        hdr.ethernet.dstAddr = 48w0xbad;
    }
    @hidden action checksum2bmv2l142() {
        hdr.ipv4.ttl = hdr.ipv4.ttl |-| 8w1;
    }
    @hidden table tbl_checksum2bmv2l126 {
        actions = {
            checksum2bmv2l126();
        }
        const default_action = checksum2bmv2l126();
    }
    @hidden table tbl_checksum2bmv2l130 {
        actions = {
            checksum2bmv2l130();
        }
        const default_action = checksum2bmv2l130();
    }
    @hidden table tbl_checksum2bmv2l134 {
        actions = {
            checksum2bmv2l134();
        }
        const default_action = checksum2bmv2l134();
    }
    @hidden table tbl_checksum2bmv2l142 {
        actions = {
            checksum2bmv2l142();
        }
        const default_action = checksum2bmv2l142();
    }
    apply {
        tbl_checksum2bmv2l126.apply();
        if (stdmeta.checksum_error == 1w1) {
            tbl_checksum2bmv2l130.apply();
        }
        if (stdmeta.parser_error != error.NoError) {
            tbl_checksum2bmv2l134.apply();
        }
        if (hdr.ipv4.isValid()) {
            tbl_checksum2bmv2l142.apply();
        }
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
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

control vc(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid(), (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
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
