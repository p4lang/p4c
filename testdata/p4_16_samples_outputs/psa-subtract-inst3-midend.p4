#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header udp_t {
    bit<16> sport;
    bit<16> dport;
    bit<16> length;
    bit<16> csum;
}

struct headers_t {
    ethernet_t ethernet;
    udp_t[2]   udp;
}

struct user_meta_data_t {
    bit<48> addr;
    bit<3>  depth1;
    bit<5>  depth2;
    bit<5>  depth3;
    bit<5>  depth4;
}

parser MyIngressParser(packet_in pkt, out headers_t hdr, inout user_meta_data_t m, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        m.depth1 = m.depth1 + 3w7;
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(inout headers_t hdrs, inout user_meta_data_t meta, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @name("MyIngressControl.nonDefAct") action nonDefAct() {
        meta.depth3 = meta.depth3 + 5w29;
    }
    @name("MyIngressControl.stub") table stub_0 {
        key = {
        }
        actions = {
            nonDefAct();
        }
        const default_action = nonDefAct();
        size = 1000000;
    }
    @hidden action psasubtractinst3l75() {
        meta.depth4 = meta.depth2 + 5w28;
        d.egress_port = (bit<32>)c.ingress_port ^ 32w1;
    }
    @hidden table tbl_psasubtractinst3l75 {
        actions = {
            psasubtractinst3l75();
        }
        const default_action = psasubtractinst3l75();
    }
    apply {
        tbl_psasubtractinst3l75.apply();
        stub_0.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psasubtractinst3l91() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psasubtractinst3l91 {
        actions = {
            psasubtractinst3l91();
        }
        const default_action = psasubtractinst3l91();
    }
    apply {
        tbl_psasubtractinst3l91.apply();
    }
}

parser MyEgressParser(packet_in pkt, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyEgressControl(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyEgressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIngressParser(), MyIngressControl(), MyIngressDeparser()) ip;

EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEgressParser(), MyEgressControl(), MyEgressDeparser()) ep;

PSA_Switch<headers_t, user_meta_data_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

