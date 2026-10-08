#include <core.p4>

#include <bmv2/psa.p4>

header Mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
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
header empty_t {
}

struct metadata_t {
}

struct headers_t {
    Tcp_option_stack options;
}

struct TcpOptionSackLength {
    bit<8> kind;
    bit<8> length;
}

parser HeaderUnionIngressParser(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit, in empty_t recirculate) {
    @name("HeaderUnionIngressParser.tmp") bit<8> tmp;
    @name("HeaderUnionIngressParser.tmp_0") bit<8> tmp_0;
    @name("HeaderUnionIngressParser.tmp_1") TcpOptionSackLength tmp_1;
    @name("HeaderUnionIngressParser.tmp_2") bit<8> tmp_2;
    @name("HeaderUnionIngressParser.tmp_3") bit<8> tmp_3;
    @name("HeaderUnionIngressParser.tmp_4") bit<32> tmp_4;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        tmp = tmp_0;
        transition select(tmp) {
            8w0x0: parse_end;
            8w0x1: parse_nop;
            8w0x2: parse_ss;
            8w0x3: parse_s;
            8w0x5: parse_sack;
            default: accept;
        }
    }
    state parse_end {
        packet.extract<Tcp_option_end_h>(hdr.options.next.end);
        transition accept;
    }
    state parse_nop {
        packet.extract<Tcp_option_nop_h>(hdr.options.next.nop);
        transition start;
    }
    state parse_ss {
        packet.extract<Tcp_option_ss_h>(hdr.options.next.ss);
        transition start;
    }
    state parse_s {
        packet.extract<Tcp_option_s_h>(hdr.options.next.s);
        transition start;
    }
    state parse_sack {
        tmp_1 = packet.lookahead<TcpOptionSackLength>();
        tmp_2 = tmp_1.length << 3;
        tmp_3 = tmp_2 + 8w240;
        tmp_4 = (bit<32>)tmp_3;
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, tmp_4);
        transition start;
    }
}

parser HeaderUnionEgressParser(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_egress_parser_input_metadata_t estd, in empty_t normal, in empty_t clone_i2e, in empty_t clone_e2e) {
    @name("HeaderUnionEgressParser.tmp_5") bit<8> tmp_5;
    @name("HeaderUnionEgressParser.tmp_6") bit<8> tmp_6;
    @name("HeaderUnionEgressParser.tmp_7") TcpOptionSackLength tmp_7;
    @name("HeaderUnionEgressParser.tmp_8") bit<8> tmp_8;
    @name("HeaderUnionEgressParser.tmp_9") bit<8> tmp_9;
    @name("HeaderUnionEgressParser.tmp_10") bit<32> tmp_10;
    state start {
        tmp_6 = packet.lookahead<bit<8>>();
        tmp_5 = tmp_6;
        transition select(tmp_5) {
            8w0x0: parse_end;
            8w0x1: parse_nop;
            8w0x2: parse_ss;
            8w0x3: parse_s;
            8w0x5: parse_sack;
            default: accept;
        }
    }
    state parse_end {
        packet.extract<Tcp_option_end_h>(hdr.options.next.end);
        transition accept;
    }
    state parse_nop {
        packet.extract<Tcp_option_nop_h>(hdr.options.next.nop);
        transition start;
    }
    state parse_ss {
        packet.extract<Tcp_option_ss_h>(hdr.options.next.ss);
        transition start;
    }
    state parse_s {
        packet.extract<Tcp_option_s_h>(hdr.options.next.s);
        transition start;
    }
    state parse_sack {
        tmp_7 = packet.lookahead<TcpOptionSackLength>();
        tmp_8 = tmp_7.length << 3;
        tmp_9 = tmp_8 + 8w240;
        tmp_10 = (bit<32>)tmp_9;
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, tmp_10);
        transition start;
    }
}

control HeaderUnionIngress(inout headers_t hdr, inout metadata_t meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("HeaderUnionIngress.meta") psa_ingress_output_metadata_t meta_0;
    @name("HeaderUnionIngress.egress_port") PortId_t egress_port_0;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)32w1;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    apply {
        send_to_port_0();
    }
}

control HeaderUnionEgress(inout headers_t hdr, inout metadata_t meta, in psa_egress_input_metadata_t estd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control HeaderUnionIngressDeparser(packet_out packet, out empty_t clone_i2e, out empty_t resubmit, out empty_t normal, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    apply {
        packet.emit<Tcp_option_h[10]>(hdr.options);
    }
}

control HeaderUnionEgressDeparser(packet_out packet, out empty_t clone_e2e, out empty_t recirculate, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t ostd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        packet.emit<Tcp_option_h[10]>(hdr.options);
    }
}

IngressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionIngressParser(), HeaderUnionIngress(), HeaderUnionIngressDeparser()) ingress;
EgressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionEgressParser(), HeaderUnionEgress(), HeaderUnionEgressDeparser()) egress;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ingress, PacketReplicationEngine(), egress, BufferingQueueingEngine()) main;
