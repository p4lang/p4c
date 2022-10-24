error {
    IPv4HeaderTooShort,
    IPv4IncorrectVersion,
    IPv4ChecksumError
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
    bit<32>     srcAddr;
    bit<32>     dstAddr;
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
    bit<4> _mystruct1_a0;
    bit<4> _mystruct1_b1;
    bool   _b2;
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
    @name("cIngress.foo") action foo() {
        hdr.tcp.srcPort = hdr.tcp.srcPort + 16w1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 32w4;
    }
    @name("cIngress.guh") table guh_0 {
        key = {
            hdr.tcp.dstPort: exact @name("hdr.tcp.dstPort");
        }
        actions = {
            foo();
        }
        default_action = foo();
    }
    apply {
        guh_0.apply();
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

struct tuple_0 {
    bit<4>      f0;
    bit<4>      f1;
    bit<8>      f2;
    bit<16>     f3;
    bit<16>     f4;
    bit<3>      f5;
    bit<13>     f6;
    bit<8>      f7;
    bit<8>      f8;
    bit<32>     f9;
    bit<32>     f10;
    varbit<320> f11;
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(true, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr,f11 = hdr.ipv4.options}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(meta._b2, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = meta._mystruct1_a0 ++ meta._mystruct1_b1,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr,f11 = hdr.ipv4.options}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
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
