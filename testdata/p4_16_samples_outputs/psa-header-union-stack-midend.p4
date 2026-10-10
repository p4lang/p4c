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

header parse_report_t {
    bit<8> parsed_count;
    bit<8> first_member;
}

struct metadata_t {
    bit<8> parsed_count;
    bit<8> first_member;
}

struct headers_t {
    parse_report_t   report;
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
        transition select((bit<1>)(meta.parsed_count == 8w0)) {
            1w1: parse_end_true;
            1w0: parse_end_join;
            default: noMatch;
        }
    }
    state parse_end_true {
        meta.first_member = 8w0;
        transition parse_end_join;
    }
    state parse_end_join {
        meta.parsed_count = meta.parsed_count + 8w1;
        transition accept;
    }
    state parse_nop {
        packet.extract<Tcp_option_nop_h>(hdr.options.next.nop);
        transition select((bit<1>)(meta.parsed_count == 8w0)) {
            1w1: parse_nop_true;
            1w0: parse_nop_join;
            default: noMatch;
        }
    }
    state parse_nop_true {
        meta.first_member = 8w1;
        transition parse_nop_join;
    }
    state parse_nop_join {
        meta.parsed_count = meta.parsed_count + 8w1;
        transition start;
    }
    state parse_ss {
        packet.extract<Tcp_option_ss_h>(hdr.options.next.ss);
        transition select((bit<1>)(meta.parsed_count == 8w0)) {
            1w1: parse_ss_true;
            1w0: parse_ss_join;
            default: noMatch;
        }
    }
    state parse_ss_true {
        meta.first_member = 8w2;
        transition parse_ss_join;
    }
    state parse_ss_join {
        meta.parsed_count = meta.parsed_count + 8w1;
        transition start;
    }
    state parse_s {
        packet.extract<Tcp_option_s_h>(hdr.options.next.s);
        transition select((bit<1>)(meta.parsed_count == 8w0)) {
            1w1: parse_s_true;
            1w0: parse_s_join;
            default: noMatch;
        }
    }
    state parse_s_true {
        meta.first_member = 8w3;
        transition parse_s_join;
    }
    state parse_s_join {
        meta.parsed_count = meta.parsed_count + 8w1;
        transition start;
    }
    state parse_sack {
        tmp_11 = packet.lookahead<bit<16>>();
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, (bit<32>)((tmp_11[7:0] << 3) + 8w240));
        transition select((bit<1>)(meta.parsed_count == 8w0)) {
            1w1: parse_sack_true;
            1w0: parse_sack_join;
            default: noMatch;
        }
    }
    state parse_sack_true {
        meta.first_member = 8w5;
        transition parse_sack_join;
    }
    state parse_sack_join {
        meta.parsed_count = meta.parsed_count + 8w1;
        transition start;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser HeaderUnionEgressParser(packet_in packet, out headers_t hdr, inout metadata_t meta, in psa_egress_parser_input_metadata_t estd, in empty_t normal, in empty_t clone_i2e, in empty_t clone_e2e) {
    @name("HeaderUnionEgressParser.tmp_6") bit<8> tmp_6;
    bit<16> tmp_12;
    state start {
        packet.extract<parse_report_t>(hdr.report);
        transition parse_options;
    }
    state parse_options {
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
        transition parse_options;
    }
    state parse_ss {
        packet.extract<Tcp_option_ss_h>(hdr.options.next.ss);
        transition parse_options;
    }
    state parse_s {
        packet.extract<Tcp_option_s_h>(hdr.options.next.s);
        transition parse_options;
    }
    state parse_sack {
        tmp_12 = packet.lookahead<bit<16>>();
        packet.extract<Tcp_option_sack_h>(hdr.options.next.sack, (bit<32>)((tmp_12[7:0] << 3) + 8w240));
        transition parse_options;
    }
}

control HeaderUnionIngress(inout headers_t hdr, inout metadata_t meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w1;
    }
    @hidden action psaheaderunionstack154() {
        hdr.report.setValid();
        hdr.report.parsed_count = meta.parsed_count;
        hdr.report.first_member = meta.first_member;
    }
    @hidden table tbl_psaheaderunionstack154 {
        actions = {
            psaheaderunionstack154();
        }
        const default_action = psaheaderunionstack154();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        tbl_psaheaderunionstack154.apply();
        tbl_send_to_port.apply();
    }
}

control HeaderUnionEgress(inout headers_t hdr, inout metadata_t meta, in psa_egress_input_metadata_t estd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control HeaderUnionIngressDeparser(packet_out packet, out empty_t clone_i2e, out empty_t resubmit, out empty_t normal, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaheaderunionstack178() {
        packet.emit<parse_report_t>(hdr.report);
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
    @hidden table tbl_psaheaderunionstack178 {
        actions = {
            psaheaderunionstack178();
        }
        const default_action = psaheaderunionstack178();
    }
    apply {
        tbl_psaheaderunionstack178.apply();
    }
}

control HeaderUnionEgressDeparser(packet_out packet, out empty_t clone_e2e, out empty_t recirculate, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t ostd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaheaderunionstack192() {
        packet.emit<parse_report_t>(hdr.report);
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
    @hidden table tbl_psaheaderunionstack192 {
        actions = {
            psaheaderunionstack192();
        }
        const default_action = psaheaderunionstack192();
    }
    apply {
        tbl_psaheaderunionstack192.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionIngressParser(), HeaderUnionIngress(), HeaderUnionIngressDeparser()) ingress;
EgressPipeline<headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t>(HeaderUnionEgressParser(), HeaderUnionEgress(), HeaderUnionEgressDeparser()) egress;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ingress, PacketReplicationEngine(), egress, BufferingQueueingEngine()) main;
