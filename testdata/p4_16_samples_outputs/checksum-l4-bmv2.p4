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
        verify(tcp_hdr_data_offset >= 5, error.TcpDataOffsetTooSmall);
        tcp_hdr_bytes_left = 4 * (bit<7>)(tcp_hdr_data_offset - 5);
        transition next_option;
    }
    state next_option {
        log_msg("tcp next_option");
        transition select(tcp_hdr_bytes_left) {
            0: accept;
            default: next_option_part2;
        }
    }
    state next_option_part2 {
        log_msg("tcp next_option_part2");
        transition select(b.lookahead<bit<8>>()) {
            0: parse_tcp_option_end;
            1: parse_tcp_option_nop;
            2: parse_tcp_option_ss;
            3: parse_tcp_option_s;
            5: parse_tcp_option_sack;
        }
    }
    state parse_tcp_option_end {
        b.extract(hdr.tcp_opt_nop);
        opt_sz = hdr.tcp_opt_nop.minSizeInBytes();
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        transition consume_remaining_tcp_hdr_and_accept;
    }
    state consume_remaining_tcp_hdr_and_accept {
        b.extract(hdr.tcp_opt_padding, (bit<32>)(8 * (bit<9>)tcp_hdr_bytes_left));
        transition accept;
    }
    state parse_tcp_option_nop {
        b.extract(hdr.tcp_opt_nop);
        opt_sz = hdr.tcp_opt_nop.minSizeInBytes();
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        transition next_option;
    }
    state parse_tcp_option_ss {
        opt_sz = hdr.tcp_opt_ss.minSizeInBytes();
        verify(tcp_hdr_bytes_left >= opt_sz, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        b.extract(hdr.tcp_opt_ss);
        opt_sz = hdr.tcp_opt_ss.minSizeInBits();
        transition next_option;
    }
    state parse_tcp_option_s {
        opt_sz = hdr.tcp_opt_s.minSizeInBytes();
        verify(tcp_hdr_bytes_left >= opt_sz, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - opt_sz;
        b.extract(hdr.tcp_opt_s);
        transition next_option;
    }
    state parse_tcp_option_sack {
        bit<8> n_sack_bytes = (b.lookahead<Tcp_option_sack_top>()).length;
        verify(n_sack_bytes == 10 || n_sack_bytes == 18 || n_sack_bytes == 26 || n_sack_bytes == 34, error.TcpBadSackOptionLength);
        verify(tcp_hdr_bytes_left >= (bit<7>)n_sack_bytes, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - (bit<7>)n_sack_bytes;
        b.extract(hdr.tcp_opt_sack, (bit<32>)(8 * n_sack_bytes - 16));
        opt_sz = hdr.tcp_opt_sack.minSizeInBits();
        transition next_option;
    }
}

parser parserI(packet_in pkt, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        pkt.extract(hdr.ipv4, (bit<32>)(8 * (4 * (bit<9>)(pkt.lookahead<IPv4_up_to_ihl_only_h>()).ihl - 20)));
        verify(hdr.ipv4.version == 4w4, error.IPv4IncorrectVersion);
        verify(hdr.ipv4.ihl >= 4w5, error.IPv4HeaderTooShort);
        meta.l4Len = hdr.ipv4.totalLen - (bit<16>)hdr.ipv4.ihl * 4;
        transition select(hdr.ipv4.protocol) {
            6: parse_tcp;
            17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        pkt.extract(hdr.tcp);
        Tcp_option_parser.apply(pkt, hdr);
        transition accept;
    }
    state parse_udp {
        pkt.extract(hdr.udp);
        transition accept;
    }
}

control cIngress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    action foot() {
        hdr.tcp.srcPort = hdr.tcp.srcPort + 1;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 4;
    }
    action foou() {
        hdr.udp.srcPort = hdr.udp.srcPort + 1;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
        hdr.ipv4.dstAddr = hdr.ipv4.dstAddr + 4;
    }
    table guh {
        key = {
            hdr.tcp.dstPort: exact;
        }
        actions = {
            foot;
            NoAction;
        }
        const default_action = NoAction;
        const entries = {
                        16w80 : foot();
        }

    }
    table huh {
        key = {
            hdr.udp.dstPort: exact;
        }
        actions = {
            foou;
            NoAction;
        }
        const default_action = NoAction;
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
        verify_checksum(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        verify_checksum_with_payload(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr, hdr.tcp_opt_ss, hdr.tcp_opt_s, hdr.tcp_opt_sack, hdr.tcp_opt_nop, hdr.tcp_opt_end }, hdr.tcp.checksum, HashAlgorithm.csum16);
        verify_checksum_with_payload(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
        update_checksum(true, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, hdr.ipv4.options }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
        update_checksum_with_payload(hdr.tcp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.tcp.srcPort, hdr.tcp.dstPort, hdr.tcp.seqNo, hdr.tcp.ackNo, hdr.tcp.dataOffset, hdr.tcp.res, hdr.tcp.ecn, hdr.tcp.ctrl, hdr.tcp.window, hdr.tcp.urgentPtr }, hdr.tcp.checksum, HashAlgorithm.csum16);
        update_checksum_with_payload(hdr.udp.isValid(), { hdr.ipv4.srcAddr, hdr.ipv4.dstAddr, 8w0, hdr.ipv4.protocol, meta.l4Len, hdr.udp.srcPort, hdr.udp.dstPort, hdr.udp.length_ }, hdr.udp.checksum, HashAlgorithm.csum16);
    }
}

control DeparserI(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
        packet.emit(hdr.tcp);
        packet.emit(hdr.tcp_opt_ss);
        packet.emit(hdr.tcp_opt_s);
        packet.emit(hdr.tcp_opt_sack);
        packet.emit(hdr.tcp_opt_nop);
        packet.emit(hdr.tcp_opt_end);
        packet.emit(hdr.tcp_opt_padding);
        packet.emit(hdr.udp);
    }
}

V1Switch<headers, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;

