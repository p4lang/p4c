error {
    TcpDataOffsetTooSmall,
    TcpOptionTooLongForHeader,
    TcpBadSackOptionLength
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

header Tcp_option_padding_h {
    varbit<256> padding;
}

struct headers {
    ethernet_t           ethernet;
    ipv4_t               ipv4;
    tcp_t                tcp;
    Tcp_option_h[10]     tcp_options_vec;
    Tcp_option_padding_h tcp_options_padding;
}

struct fwd_metadata_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

struct metadata {
    bit<32> _fwd_metadata_l2ptr0;
    bit<24> _fwd_metadata_out_bd1;
}

struct Tcp_option_sack_top {
    bit<8> kind;
    bit<8> length;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.Tcp_option_parser.tcp_hdr_bytes_left") bit<7> Tcp_option_parser_tcp_hdr_bytes_left;
    @name("ParserImpl.Tcp_option_parser.tmp_0") bit<8> Tcp_option_parser_tmp_0;
    bit<16> tmp;
    state start {
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
        hdr.tcp_options_vec[0].end.setInvalid();
        hdr.tcp_options_vec[0].nop.setInvalid();
        hdr.tcp_options_vec[0].ss.setInvalid();
        hdr.tcp_options_vec[0].s.setInvalid();
        hdr.tcp_options_vec[0].sack.setInvalid();
        hdr.tcp_options_vec[1].end.setInvalid();
        hdr.tcp_options_vec[1].nop.setInvalid();
        hdr.tcp_options_vec[1].ss.setInvalid();
        hdr.tcp_options_vec[1].s.setInvalid();
        hdr.tcp_options_vec[1].sack.setInvalid();
        hdr.tcp_options_vec[2].end.setInvalid();
        hdr.tcp_options_vec[2].nop.setInvalid();
        hdr.tcp_options_vec[2].ss.setInvalid();
        hdr.tcp_options_vec[2].s.setInvalid();
        hdr.tcp_options_vec[2].sack.setInvalid();
        hdr.tcp_options_vec[3].end.setInvalid();
        hdr.tcp_options_vec[3].nop.setInvalid();
        hdr.tcp_options_vec[3].ss.setInvalid();
        hdr.tcp_options_vec[3].s.setInvalid();
        hdr.tcp_options_vec[3].sack.setInvalid();
        hdr.tcp_options_vec[4].end.setInvalid();
        hdr.tcp_options_vec[4].nop.setInvalid();
        hdr.tcp_options_vec[4].ss.setInvalid();
        hdr.tcp_options_vec[4].s.setInvalid();
        hdr.tcp_options_vec[4].sack.setInvalid();
        hdr.tcp_options_vec[5].end.setInvalid();
        hdr.tcp_options_vec[5].nop.setInvalid();
        hdr.tcp_options_vec[5].ss.setInvalid();
        hdr.tcp_options_vec[5].s.setInvalid();
        hdr.tcp_options_vec[5].sack.setInvalid();
        hdr.tcp_options_vec[6].end.setInvalid();
        hdr.tcp_options_vec[6].nop.setInvalid();
        hdr.tcp_options_vec[6].ss.setInvalid();
        hdr.tcp_options_vec[6].s.setInvalid();
        hdr.tcp_options_vec[6].sack.setInvalid();
        hdr.tcp_options_vec[7].end.setInvalid();
        hdr.tcp_options_vec[7].nop.setInvalid();
        hdr.tcp_options_vec[7].ss.setInvalid();
        hdr.tcp_options_vec[7].s.setInvalid();
        hdr.tcp_options_vec[7].sack.setInvalid();
        hdr.tcp_options_vec[8].end.setInvalid();
        hdr.tcp_options_vec[8].nop.setInvalid();
        hdr.tcp_options_vec[8].ss.setInvalid();
        hdr.tcp_options_vec[8].s.setInvalid();
        hdr.tcp_options_vec[8].sack.setInvalid();
        hdr.tcp_options_vec[9].end.setInvalid();
        hdr.tcp_options_vec[9].nop.setInvalid();
        hdr.tcp_options_vec[9].ss.setInvalid();
        hdr.tcp_options_vec[9].s.setInvalid();
        hdr.tcp_options_vec[9].sack.setInvalid();
        hdr.tcp_options_padding.setInvalid();
        verify(hdr.tcp.dataOffset >= 4w5, error.TcpDataOffsetTooSmall);
        Tcp_option_parser_tcp_hdr_bytes_left = (bit<7>)(hdr.tcp.dataOffset + 4w11) << 2;
        transition Tcp_option_parser_next_option;
    }
    state Tcp_option_parser_next_option {
        transition select(Tcp_option_parser_tcp_hdr_bytes_left) {
            7w0: parse_tcp_0;
            default: Tcp_option_parser_next_option_part2;
        }
    }
    state Tcp_option_parser_next_option_part2 {
        Tcp_option_parser_tmp_0 = packet.lookahead<bit<8>>();
        transition select(Tcp_option_parser_tmp_0) {
            8w0: Tcp_option_parser_parse_tcp_option_end;
            8w1: Tcp_option_parser_parse_tcp_option_nop;
            8w2: Tcp_option_parser_parse_tcp_option_ss;
            8w3: Tcp_option_parser_parse_tcp_option_s;
            8w5: Tcp_option_parser_parse_tcp_option_sack;
            default: noMatch;
        }
    }
    state Tcp_option_parser_parse_tcp_option_end {
        packet.extract<Tcp_option_end_h>(hdr.tcp_options_vec.next.end);
        Tcp_option_parser_tcp_hdr_bytes_left = Tcp_option_parser_tcp_hdr_bytes_left + 7w127;
        packet.extract<Tcp_option_padding_h>(hdr.tcp_options_padding, (bit<32>)((bit<9>)Tcp_option_parser_tcp_hdr_bytes_left << 3));
        transition parse_tcp_0;
    }
    state Tcp_option_parser_parse_tcp_option_nop {
        packet.extract<Tcp_option_nop_h>(hdr.tcp_options_vec.next.nop);
        Tcp_option_parser_tcp_hdr_bytes_left = Tcp_option_parser_tcp_hdr_bytes_left + 7w127;
        transition Tcp_option_parser_next_option;
    }
    state Tcp_option_parser_parse_tcp_option_ss {
        verify(Tcp_option_parser_tcp_hdr_bytes_left >= 7w5, error.TcpOptionTooLongForHeader);
        Tcp_option_parser_tcp_hdr_bytes_left = Tcp_option_parser_tcp_hdr_bytes_left + 7w123;
        packet.extract<Tcp_option_ss_h>(hdr.tcp_options_vec.next.ss);
        transition Tcp_option_parser_next_option;
    }
    state Tcp_option_parser_parse_tcp_option_s {
        verify(Tcp_option_parser_tcp_hdr_bytes_left >= 7w4, error.TcpOptionTooLongForHeader);
        Tcp_option_parser_tcp_hdr_bytes_left = Tcp_option_parser_tcp_hdr_bytes_left + 7w124;
        packet.extract<Tcp_option_s_h>(hdr.tcp_options_vec.next.s);
        transition Tcp_option_parser_next_option;
    }
    state Tcp_option_parser_parse_tcp_option_sack {
        tmp = packet.lookahead<bit<16>>();
        verify(tmp[7:0] == 8w10 || tmp[7:0] == 8w18 || tmp[7:0] == 8w26 || tmp[7:0] == 8w34, error.TcpBadSackOptionLength);
        verify(Tcp_option_parser_tcp_hdr_bytes_left >= (bit<7>)tmp[7:0], error.TcpOptionTooLongForHeader);
        Tcp_option_parser_tcp_hdr_bytes_left = Tcp_option_parser_tcp_hdr_bytes_left - (bit<7>)tmp[7:0];
        packet.extract<Tcp_option_sack_h>(hdr.tcp_options_vec.next.sack, (bit<32>)((tmp[7:0] << 3) + 8w240));
        transition Tcp_option_parser_next_option;
    }
    state parse_tcp_0 {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.smeta") standard_metadata_t smeta_0;
    @name("ingress.smeta") standard_metadata_t smeta_3;
    @name(".my_drop") action my_drop_2() {
        smeta_0.ingress_port = standard_metadata.ingress_port;
        smeta_0.egress_spec = standard_metadata.egress_spec;
        smeta_0.egress_port = standard_metadata.egress_port;
        smeta_0.instance_type = standard_metadata.instance_type;
        smeta_0.packet_length = standard_metadata.packet_length;
        smeta_0.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_0.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_0.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_0.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_0.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_0.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_0.mcast_grp = standard_metadata.mcast_grp;
        smeta_0.egress_rid = standard_metadata.egress_rid;
        smeta_0.checksum_error = standard_metadata.checksum_error;
        smeta_0.parser_error = standard_metadata.parser_error;
        smeta_0.priority = standard_metadata.priority;
        mark_to_drop(smeta_0);
        standard_metadata.ingress_port = smeta_0.ingress_port;
        standard_metadata.egress_spec = smeta_0.egress_spec;
        standard_metadata.egress_port = smeta_0.egress_port;
        standard_metadata.instance_type = smeta_0.instance_type;
        standard_metadata.packet_length = smeta_0.packet_length;
        standard_metadata.enq_timestamp = smeta_0.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_0.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_0.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_0.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_0.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_0.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_0.mcast_grp;
        standard_metadata.egress_rid = smeta_0.egress_rid;
        standard_metadata.checksum_error = smeta_0.checksum_error;
        standard_metadata.parser_error = smeta_0.parser_error;
        standard_metadata.priority = smeta_0.priority;
    }
    @name(".my_drop") action my_drop_3() {
        smeta_3.ingress_port = standard_metadata.ingress_port;
        smeta_3.egress_spec = standard_metadata.egress_spec;
        smeta_3.egress_port = standard_metadata.egress_port;
        smeta_3.instance_type = standard_metadata.instance_type;
        smeta_3.packet_length = standard_metadata.packet_length;
        smeta_3.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_3.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_3.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_3.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_3.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_3.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_3.mcast_grp = standard_metadata.mcast_grp;
        smeta_3.egress_rid = standard_metadata.egress_rid;
        smeta_3.checksum_error = standard_metadata.checksum_error;
        smeta_3.parser_error = standard_metadata.parser_error;
        smeta_3.priority = standard_metadata.priority;
        mark_to_drop(smeta_3);
        standard_metadata.ingress_port = smeta_3.ingress_port;
        standard_metadata.egress_spec = smeta_3.egress_spec;
        standard_metadata.egress_port = smeta_3.egress_port;
        standard_metadata.instance_type = smeta_3.instance_type;
        standard_metadata.packet_length = smeta_3.packet_length;
        standard_metadata.enq_timestamp = smeta_3.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_3.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_3.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_3.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_3.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_3.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_3.mcast_grp;
        standard_metadata.egress_rid = smeta_3.egress_rid;
        standard_metadata.checksum_error = smeta_3.checksum_error;
        standard_metadata.parser_error = smeta_3.parser_error;
        standard_metadata.priority = smeta_3.priority;
    }
    @name("ingress.set_l2ptr") action set_l2ptr(@name("l2ptr") bit<32> l2ptr_1) {
        meta._fwd_metadata_l2ptr0 = l2ptr_1;
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            set_l2ptr();
            my_drop_2();
        }
        default_action = my_drop_2();
    }
    @name("ingress.set_bd_dmac_intf") action set_bd_dmac_intf(@name("bd") bit<24> bd, @name("dmac") bit<48> dmac, @name("intf") bit<9> intf) {
        meta._fwd_metadata_out_bd1 = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            meta._fwd_metadata_l2ptr0: exact @name("meta.fwd_metadata.l2ptr");
        }
        actions = {
            set_bd_dmac_intf();
            my_drop_3();
        }
        default_action = my_drop_3();
    }
    apply {
        ipv4_da_lpm_0.apply();
        mac_da_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("egress.smeta") standard_metadata_t smeta_4;
    @name(".my_drop") action my_drop_4() {
        smeta_4.ingress_port = standard_metadata.ingress_port;
        smeta_4.egress_spec = standard_metadata.egress_spec;
        smeta_4.egress_port = standard_metadata.egress_port;
        smeta_4.instance_type = standard_metadata.instance_type;
        smeta_4.packet_length = standard_metadata.packet_length;
        smeta_4.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_4.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_4.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_4.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_4.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_4.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_4.mcast_grp = standard_metadata.mcast_grp;
        smeta_4.egress_rid = standard_metadata.egress_rid;
        smeta_4.checksum_error = standard_metadata.checksum_error;
        smeta_4.parser_error = standard_metadata.parser_error;
        smeta_4.priority = standard_metadata.priority;
        mark_to_drop(smeta_4);
        standard_metadata.ingress_port = smeta_4.ingress_port;
        standard_metadata.egress_spec = smeta_4.egress_spec;
        standard_metadata.egress_port = smeta_4.egress_port;
        standard_metadata.instance_type = smeta_4.instance_type;
        standard_metadata.packet_length = smeta_4.packet_length;
        standard_metadata.enq_timestamp = smeta_4.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_4.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_4.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_4.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_4.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_4.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_4.mcast_grp;
        standard_metadata.egress_rid = smeta_4.egress_rid;
        standard_metadata.checksum_error = smeta_4.checksum_error;
        standard_metadata.parser_error = smeta_4.parser_error;
        standard_metadata.priority = smeta_4.priority;
    }
    @name("egress.rewrite_mac") action rewrite_mac(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egress.send_frame") table send_frame_0 {
        key = {
            meta._fwd_metadata_out_bd1: exact @name("meta.fwd_metadata.out_bd");
        }
        actions = {
            rewrite_mac();
            my_drop_4();
        }
        default_action = my_drop_4();
    }
    apply {
        send_frame_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
        packet.emit<tcp_t>(hdr.tcp);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[0].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[0].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[0].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[0].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[0].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[1].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[1].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[1].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[1].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[1].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[2].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[2].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[2].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[2].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[2].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[3].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[3].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[3].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[3].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[3].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[4].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[4].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[4].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[4].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[4].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[5].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[5].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[5].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[5].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[5].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[6].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[6].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[6].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[6].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[6].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[7].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[7].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[7].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[7].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[7].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[8].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[8].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[8].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[8].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[8].sack);
        packet.emit<Tcp_option_end_h>(hdr.tcp_options_vec[9].end);
        packet.emit<Tcp_option_nop_h>(hdr.tcp_options_vec[9].nop);
        packet.emit<Tcp_option_ss_h>(hdr.tcp_options_vec[9].ss);
        packet.emit<Tcp_option_s_h>(hdr.tcp_options_vec[9].s);
        packet.emit<Tcp_option_sack_h>(hdr.tcp_options_vec[9].sack);
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
