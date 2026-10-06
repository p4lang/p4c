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

header empty_t {
}

struct metadata_t {
}

struct headers_t {
    Tcp_option_h[10] options;
}

struct TcpOptionSackLength {
    bit<8> kind;
    bit<8> length;
}

parser HeaderUnionIngressParser(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit, in empty_t recirculate) {
    @name("HeaderUnionIngressParser.tmp_0") bit<8> tmp_0;
    bit<16> tmp_11;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        transition select(tmp_0) {
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
        tmp_11 = packet.lookahead<bit<16>>();
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, (bit<32>)((tmp_11[7:0] << 3) + 8w240));
        transition start;
    }
}

parser HeaderUnionEgressParser(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_egress_parser_input_metadata_t estd, in empty_t normal, in empty_t clone_i2e, in empty_t clone_e2e) {
    @name("HeaderUnionEgressParser.tmp_6") bit<8> tmp_6;
    bit<16> tmp_12;
    state start {
        tmp_6 = packet.lookahead<bit<8>>();
        transition select(tmp_6) {
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
        tmp_12 = packet.lookahead<bit<16>>();
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, (bit<32>)((tmp_12[7:0] << 3) + 8w240));
        transition start;
    }
}

control HeaderUnionIngress(inout headers_t hdr, inout metadata_t meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w1;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        tbl_send_to_port.apply();
    }
}

control HeaderUnionEgress(inout headers_t hdr, inout metadata_t meta, in psa_egress_input_metadata_t estd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control HeaderUnionIngressDeparser(packet_out packet, out empty_t clone_i2e, out empty_t resubmit, out empty_t normal, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaheaderunionstack142() {
        packet.emit<Tcp_option_end_h>(hdr.options[0].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[0].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[0].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[0].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[0].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[1].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[1].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[1].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[1].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[1].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[2].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[2].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[2].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[2].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[2].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[3].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[3].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[3].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[3].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[3].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[4].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[4].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[4].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[4].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[4].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[5].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[5].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[5].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[5].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[5].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[6].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[6].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[6].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[6].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[6].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[7].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[7].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[7].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[7].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[7].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[8].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[8].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[8].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[8].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[8].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[9].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[9].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[9].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[9].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[9].sack);
    }
    @hidden table tbl_psaheaderunionstack142 {
        actions = {
            psaheaderunionstack142();
        }
        const default_action = psaheaderunionstack142();
    }
    apply {
        tbl_psaheaderunionstack142.apply();
    }
}

control HeaderUnionEgressDeparser(packet_out packet, out empty_t clone_e2e, out empty_t recirculate, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t ostd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaheaderunionstack164() {
        packet.emit<Tcp_option_end_h>(hdr.options[0].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[0].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[0].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[0].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[0].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[1].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[1].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[1].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[1].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[1].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[2].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[2].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[2].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[2].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[2].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[3].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[3].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[3].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[3].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[3].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[4].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[4].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[4].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[4].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[4].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[5].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[5].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[5].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[5].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[5].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[6].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[6].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[6].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[6].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[6].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[7].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[7].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[7].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[7].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[7].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[8].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[8].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[8].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[8].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[8].sack);
        packet.emit<Tcp_option_end_h>(hdr.options[9].end);
        packet.emit<Tcp_option_nop_h>(hdr.options[9].nop);
        packet.emit<Tcp_option_ss_h>(hdr.options[9].ss);
        packet.emit<Tcp_option_s_h>(hdr.options[9].s);
        packet.emit<Tcp_option_sack_h>(hdr.options[9].sack);
    }
    @hidden table tbl_psaheaderunionstack164 {
        actions = {
            psaheaderunionstack164();
        }
        const default_action = psaheaderunionstack164();
    }
    apply {
        tbl_psaheaderunionstack164.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionIngressParser(), HeaderUnionIngress(), HeaderUnionIngressDeparser()) ingress;
EgressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionEgressParser(), HeaderUnionEgress(), HeaderUnionEgressDeparser()) egress;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ingress, PacketReplicationEngine(), egress, BufferingQueueingEngine()) main;
