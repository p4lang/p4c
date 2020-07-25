error {
    TcpDataOffsetTooSmall,
    TcpOptionTooLongForHeader,
    TcpBadSackOptionLength,
    IPv4HeaderTooShort,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}
#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> EthernetAddress;
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

header Tcp_option_end_h {
    bit<8> kind;
}

header Tcp_option_nop_h {
    bit<8> kind;
}

header Tcp_option_ss_h {
    bit<8>  kind;
    bit<32> maxSegmentSize;
}

header Tcp_option_s_h {
    bit<8>  kind;
    bit<24> scale;
}

header Tcp_option_sack_h {
    bit<8>      kind;
    bit<8>      length;
    varbit<256> sack;
}

header Tcp_option_padding_h {
    varbit<256> padding;
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

struct headers {
    ethernet_t           ethernet;
    ipv4_t               ipv4;
    tcp_t                tcp;
    Tcp_option_ss_h      tcp_opt_ss;
    Tcp_option_s_h       tcp_opt_s;
    Tcp_option_sack_h    tcp_opt_sack;
    Tcp_option_end_h     tcp_opt_end;
    Tcp_option_nop_h     tcp_opt_nop;
    Tcp_option_padding_h tcp_opt_padding;
    udp_t                udp;
}

struct Tcp_option_sack_top {
    bit<8> kind;
    bit<8> length;
}

struct mystruct1_t {
    bit<4> a;
    bit<4> b;
}

struct metadata {
    mystruct1_t mystruct1;
    bit<16>     l4Len;
}

typedef tuple<bit<4>, bit<4>, bit<8>, varbit<56>> myTuple1;
parser Tcp_option_parser(packet_in b, inout headers hdr) {
    bit<7> tcp_hdr_bytes_left;
    bit<7> opt_sz;
    bit<4> tcp_hdr_data_offset = hdr.tcp.dataOffset;
    state start {
        verify(tcp_hdr_data_offset >= 4w5, error.TcpDataOffsetTooSmall);
        tcp_hdr_bytes_left = (bit<7>)(tcp_hdr_data_offset + 4w11) << 2;
        transition next_option;
    }
    state next_option {
        log_msg("tcp next_option");
        transition select(tcp_hdr_bytes_left) {
            7w0: accept;
            default: next_option_part2;
        }
    }
    state next_option_part2 {
        log_msg("tcp next_option_part2");
        transition select(b.lookahead<bit<8>>()) {
            8w0: parse_tcp_option_end;
            8w1: parse_tcp_option_nop;
            8w2: parse_tcp_option_ss;
            8w3: parse_tcp_option_s;
            8w5: parse_tcp_option_sack;
        }
    }
    state parse_tcp_option_end {
        b.extract<Tcp_option_nop_h>(hdr.tcp_opt_nop);
        opt_sz = 7w1;
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        transition consume_remaining_tcp_hdr_and_accept;
    }
    state consume_remaining_tcp_hdr_and_accept {
        b.extract<Tcp_option_padding_h>(hdr.tcp_opt_padding, (bit<32>)((bit<9>)tcp_hdr_bytes_left << 3));
        transition accept;
    }
    state parse_tcp_option_nop {
        b.extract<Tcp_option_nop_h>(hdr.tcp_opt_nop);
        opt_sz = 7w1;
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        transition next_option;
    }
    state parse_tcp_option_ss {
        opt_sz = 7w5;
        verify(tcp_hdr_bytes_left >= opt_sz, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        b.extract<Tcp_option_ss_h>(hdr.tcp_opt_ss);
        opt_sz = 7w40;
        transition next_option;
    }
    state parse_tcp_option_s {
        opt_sz = 7w4;
        verify(tcp_hdr_bytes_left >= opt_sz, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        b.extract<Tcp_option_s_h>(hdr.tcp_opt_s);
        transition next_option;
    }
    state parse_tcp_option_sack {
        bit<8> n_sack_bytes = (b.lookahead<Tcp_option_sack_top>()).length;
        verify(n_sack_bytes == 8w10 || n_sack_bytes == 8w18 || n_sack_bytes == 8w26 || n_sack_bytes == 8w34, error.TcpBadSackOptionLength);
        verify(tcp_hdr_bytes_left >= (bit<7>)n_sack_bytes, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - (bit<7>)n_sack_bytes;
        b.extract<Tcp_option_sack_h>(hdr.tcp_opt_sack, (bit<32>)((n_sack_bytes << 3) + 8w240));
        opt_sz = 7w16;
        transition next_option;
    }
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @name("Tcp_option_parser") Tcp_option_parser() Tcp_option_parser_inst;
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract<ipv4_t>(hdr.ipv4, (bit<32>)(((bit<9>)(pkt.lookahead<IPv4_up_to_ihl_only_h>()).ihl << 2) + 9w492 << 3));
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl >= 4w5, error.IPv4HeaderTooShort);
        meta.l4Len = hdr.ipv4.totalLen - ((bit<16>)hdr.ipv4.ihl << 2);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract<tcp_t>(hdr.tcp);
        Tcp_option_parser_inst.apply(pkt, hdr);
        transition accept;
    }
    state parse_udp {
        pkt.extract<udp_t>(hdr.udp);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    action foot() {
        hdr.tcp.srcPort = hdr.tcp.srcPort + 16w1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 32w4;
    }
    action foou() {
        hdr.udp.srcPort = hdr.udp.srcPort + 16w1;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 32w4;
    }
    table guh {
        key = {
            hdr.tcp.dstPort: exact @name("hdr.tcp.dstPort") ;
        }
        actions = {
            foot();
            NoAction();
        }
        const default_action = NoAction();
        const entries = {
                        16w80 : foot();
        }

    }
    table huh {
        key = {
            hdr.udp.dstPort: exact @name("hdr.udp.dstPort") ;
        }
        actions = {
            foou();
            NoAction();
        }
        const default_action = NoAction();
        const entries = {
                        16w80 : foou();
        }

    }
    apply {
        if (hdr.tcp.isValid()) {
            guh.apply();
        }
        if (hdr.udp.isValid()) {
            huh.apply();
        }
    }
}

control cEgress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>, varbit<320>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<32>, bit<32>, bit<4>, bit<3>, bit<3>, bit<6>, bit<16>, bit<16>, Tcp_option_ss_h, Tcp_option_s_h, Tcp_option_sack_h, Tcp_option_nop_h, Tcp_option_end_h>, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp_opt_ss, hdr.tcp_opt_s, hdr.tcp_opt_sack, hdr.tcp_opt_nop, hdr.tcp_opt_end }, hdr.tcp.checksum, HashAlgorithm.csum16);
        verify_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<16>>, bit<16>>(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>, varbit<320>>, bit<16>>(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<32>, bit<32>, bit<4>, bit<3>, bit<3>, bit<6>, bit<16>, bit<16>>, bit<16>>(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr }, hdr.tcp.checksum, HashAlgorithm.csum16);
        update_checksum_with_payload<tuple<bit<32>, bit<32>, bit<8>, bit<8>, bit<16>, bit<16>, bit<16>, bit<16>>, bit<16>>(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_opt_ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_opt_s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_opt_sack);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_opt_nop);
        packet.emit<Tcp_option_end_h>(hdr.tcp_opt_end);
        packet.emit<Tcp_option_padding_h>(hdr.tcp_opt_padding);
        packet.emit<udp_t>(hdr.udp);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

