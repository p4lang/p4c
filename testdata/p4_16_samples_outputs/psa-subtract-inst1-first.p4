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
    bit<5>  depth5;
    bit<5>  depth6;
    bit<5>  depth7;
}

parser MyIngressParser(packet_in pkt, out headers_t hdr, inout user_meta_data_t m, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        m.depth1 = m.depth1 + 3w7;
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(inout headers_t hdrs, inout user_meta_data_t meta, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    bit<5> var1 = 5w8;
    bit<5> var2 = 5w2;
    int<33> var3;
    int<33> var4 = var3 + -33s3;
    int<33> var5 = var4 - var3;
    action nonDefAct() {
        if (var4 == 33s3 && var5 == 33s1) {
            meta.depth3 = meta.depth3 + 5w29;
            meta.depth4 = var1 + var2;
            meta.depth5 = var1 ^ 5w2;
            meta.depth6 = var2 + 5w31;
            meta.depth7 = var2 + 5w3;
        }
    }
    table stub {
        actions = {
            nonDefAct();
        }
        const default_action = nonDefAct();
        size = 1000000;
    }
    apply {
        meta.depth4 = meta.depth2 + 5w28;
        d.egress_port = (PortId_t)((bit<32>)c.ingress_port ^ 32w1);
        stub.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
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
