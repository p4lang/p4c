error {
    TcpDataOffsetTooSmall,
    TcpOptionTooLongForHeader,
    TcpBadSackOptionLength
}
#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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

header_union Tcp_option_h {
    Tcp_option_end_h  end;
    Tcp_option_nop_h  nop;
    Tcp_option_ss_h   ss;
    Tcp_option_s_h    s;
    Tcp_option_sack_h sack;
}

typedef Tcp_option_h[10] Tcp_option_stack;
header Tcp_option_padding_h {
    varbit<256> padding;
}

struct headers {
    ethernet_t           ethernet;
    ipv4_t               ipv4;
    tcp_t                tcp;
    Tcp_option_stack     tcp_options_vec;
    Tcp_option_padding_h tcp_options_padding;
}

struct fwd_metadata_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

struct metadata {
    fwd_metadata_t fwd_metadata;
}

struct Tcp_option_sack_top {
    bit<8> kind;
    bit<8> length;
}

parser Tcp_option_parser(packet_in b, in bit<4> tcp_hdr_data_offset, out Tcp_option_stack vec, out Tcp_option_padding_h padding) {
    bit<7> tcp_hdr_bytes_left;
    state start {
        verify(tcp_hdr_data_offset >= 4w5, error.TcpDataOffsetTooSmall);
        tcp_hdr_bytes_left = (bit<7>)(tcp_hdr_data_offset + 4w11) << 2;
        transition next_option;
    }
    state next_option {
        transition select(tcp_hdr_bytes_left) {
            7w0: accept;
            default: next_option_part2;
        }
    }
    state next_option_part2 {
        transition select(b.lookahead<bit<8>>()) {
            8w0: parse_tcp_option_end;
            8w1: parse_tcp_option_nop;
            8w2: parse_tcp_option_ss;
            8w3: parse_tcp_option_s;
            8w5: parse_tcp_option_sack;
        }
    }
    state parse_tcp_option_end {
        b.extract<Tcp_option_end_h>(vec.next.end);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left + 7w127;
        transition consume_remaining_tcp_hdr_and_accept;
    }
    state consume_remaining_tcp_hdr_and_accept {
        b.extract<Tcp_option_padding_h>(padding, (bit<32>)((bit<9>)tcp_hdr_bytes_left << 3));
        transition accept;
    }
    state parse_tcp_option_nop {
        b.extract<Tcp_option_nop_h>(vec.next.nop);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left + 7w127;
        transition next_option;
    }
    state parse_tcp_option_ss {
        verify(tcp_hdr_bytes_left >= 7w5, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left + 7w123;
        b.extract<Tcp_option_ss_h>(vec.next.ss);
        transition next_option;
    }
    state parse_tcp_option_s {
        verify(tcp_hdr_bytes_left >= 7w4, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left + 7w124;
        b.extract<Tcp_option_s_h>(vec.next.s);
        transition next_option;
    }
    state parse_tcp_option_sack {
        bit<8> n_sack_bytes = (b.lookahead<Tcp_option_sack_top>()).length;
        verify(n_sack_bytes == 8w10 || n_sack_bytes == 8w18 || n_sack_bytes == 8w26 || n_sack_bytes == 8w34, error.TcpBadSackOptionLength);
        verify(tcp_hdr_bytes_left >= (bit<7>)n_sack_bytes, error.TcpOptionTooLongForHeader);
        tcp_hdr_bytes_left = tcp_hdr_bytes_left - (bit<7>)n_sack_bytes;
        b.extract<Tcp_option_sack_h>(vec.next.sack, (bit<32>)((n_sack_bytes << 3) + 8w240));
        transition next_option;
    }
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("Tcp_option_parser") Tcp_option_parser() Tcp_option_parser_inst;
    const bit<16> ETHERTYPE_IPV4 = 16w0x800;
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            8w6: parse_tcp;
            default: accept;
        }
    }
    state parse_tcp {
        packet.extract<tcp_t>(hdr.tcp);
        Tcp_option_parser_inst.apply(packet, hdr.tcp.dataOffset, hdr.tcp_options_vec, hdr.tcp_options_padding);
        transition accept;
    }
}

action my_drop(inout standard_metadata_t smeta) {
    mark_to_drop(smeta);
}
control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action set_l2ptr(bit<32> l2ptr) {
        meta.fwd_metadata.l2ptr = l2ptr;
    }
    table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_l2ptr();
            my_drop(standard_metadata);
        }
        default_action = my_drop(standard_metadata);
    }
    action set_bd_dmac_intf(bit<24> bd, bit<48> dmac, bit<9> intf) {
        meta.fwd_metadata.out_bd = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    table mac_da {
        key = {
            meta.fwd_metadata.l2ptr: exact @name("meta.fwd_metadata.l2ptr") ;
        }
        actions = {
            set_bd_dmac_intf();
            my_drop(standard_metadata);
        }
        default_action = my_drop(standard_metadata);
    }
    apply {
        ipv4_da_lpm.apply();
        mac_da.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    table send_frame {
        key = {
            meta.fwd_metadata.out_bd: exact @name("meta.fwd_metadata.out_bd") ;
        }
        actions = {
            rewrite_mac();
            my_drop(standard_metadata);
        }
        default_action = my_drop(standard_metadata);
    }
    apply {
        send_frame.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<Tcp_option_h[10]>(hdr.tcp_options_vec);
        packet.emit<Tcp_option_padding_h>(hdr.tcp_options_padding);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

