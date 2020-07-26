error {
    IPv4HeaderTooShort,
    TCPHeaderTooShort,
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
    bit<16>     srcPort;
    bit<16>     dstPort;
    bit<32>     seqNo;
    bit<32>     ackNo;
    bit<4>      dataOffset;
    bit<3>      res;
    bit<3>      ecn;
    bit<6>      ctrl;
    bit<16>     window;
    bit<16>     checksum;
    bit<16>     urgentPtr;
    varbit<320> options;
}

header udp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<16> length_;
    bit<16> checksum;
}

header IPv4_up_to_ihl_only_h {
    bit<4> version;
    bit<4> ihl;
}

header tcp_upto_data_offset_only_h {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<4>  dontCare;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
    tcp_t      tcp;
    udp_t      udp;
}

struct mystruct1_t {
    bit<4> a;
    bit<4> b;
}

struct metadata {
    bit<4>  _mystruct1_a0;
    bit<4>  _mystruct1_b1;
    bit<16> _l4Len2;
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    IPv4_up_to_ihl_only_h tmp;
    tcp_upto_data_offset_only_h tmp_4;
    bit<8> tmp_9;
    bit<104> tmp_10;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        tmp_9 = pkt.lookahead<bit<8>>();
        tmp.setValid();
        tmp.setValid();
        tmp.version = tmp_9[7:4];
        tmp.ihl = tmp_9[3:0];
        pkt.extract<ipv4_t>(hdr.ipv4, (bit<32>)(((bit<9>)tmp_9[3:0] << 2) + 9w492 << 3));
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl >= 4w5, error.IPv4HeaderTooShort);
        meta._l4Len2 = hdr.ipv4.totalLen - ((bit<16>)hdr.ipv4.ihl << 2);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        tmp_10 = pkt.lookahead<bit<104>>();
        tmp_4.setValid();
        tmp_4.setValid();
        tmp_4.srcPort = tmp_10[103:88];
        tmp_4.dstPort = tmp_10[87:72];
        tmp_4.seqNo = tmp_10[71:40];
        tmp_4.ackNo = tmp_10[39:8];
        tmp_4.dataOffset = tmp_10[7:4];
        tmp_4.dontCare = tmp_10[3:0];
        pkt.extract<tcp_t>(hdr.tcp, (bit<32>)(((bit<9>)tmp_10[7:4] << 2) + 9w492 << 3));
        verify(hdr.tcp.dataOffset >= 4w5, error.TCPHeaderTooShort);
        transition accept;
    }
    state parse_udp {
        pkt.extract<udp_t>(hdr.udp);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("cIngress.foot") action foot() {
        hdr.tcp.srcPort = hdr.tcp.srcPort + 16w1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 32w4;
    }
    @name("cIngress.foou") action foou() {
        hdr.udp.srcPort = hdr.udp.srcPort + 16w1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 32w4;
    }
    @name("cIngress.guh") table guh_0 {
        key = {
            hdr.tcp.dstPort: exact @name("hdr.tcp.dstPort") ;
        }
        actions = {
            foot();
            NoAction_0();
        }
        const default_action = NoAction_0();
        const entries = {
                        16w80 : foot();
        }

    }
    @name("cIngress.huh") table huh_0 {
        key = {
            hdr.udp.dstPort: exact @name("hdr.udp.dstPort") ;
        }
        actions = {
            foou();
            NoAction_3();
        }
        const default_action = NoAction_3();
        const entries = {
                        16w80 : foou();
        }

    }
    apply {
        if (hdr.tcp.isValid()) {
            guh_0.apply();
        }
        if (hdr.udp.isValid()) {
            huh_0.apply();
        }
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

struct tuple_0 {
    bit<4>      field;
    bit<4>      field_0;
    bit<8>      field_1;
    bit<16>     field_2;
    bit<16>     field_3;
    bit<3>      field_4;
    bit<13>     field_5;
    bit<8>      field_6;
    bit<8>      field_7;
    bit<32>     field_8;
    bit<32>     field_9;
    varbit<320> field_10;
}

struct tuple_1 {
    bit<32>     field_11;
    bit<32>     field_12;
    bit<8>      field_13;
    bit<8>      field_14;
    bit<16>     field_15;
    bit<16>     field_16;
    bit<16>     field_17;
    bit<32>     field_18;
    bit<32>     field_19;
    bit<4>      field_20;
    bit<3>      field_21;
    bit<3>      field_22;
    bit<6>      field_23;
    bit<16>     field_24;
    bit<16>     field_25;
    varbit<320> field_26;
}

struct tuple_2 {
    bit<32> field_27;
    bit<32> field_28;
    bit<8>  field_29;
    bit<8>  field_30;
    bit<16> field_31;
    bit<16> field_32;
    bit<16> field_33;
    bit<16> field_34;
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple_0, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple_1, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta._l4Len2, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options }, hdr.tcp.checksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple_2, bit<16>>(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta._l4Len2, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple_1, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta._l4Len2, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp.options }, hdr.tcp.checksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple_2, bit<16>>(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta._l4Len2, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<udp_t>(hdr.udp);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

