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
    bit<3>  depth2;
    bit<3>  depth3;
    bit<3>  depth4;
}

parser MyIngressParser(packet_in pkt, out headers_t hdr, inout user_meta_data_t m, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyIngressControl(inout headers_t hdrs, inout user_meta_data_t meta, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    bit<3> var1 = 3w0;
    bit<3> var2 = 3w2;
    int<3> var3;
    int<3> var4 = var3 + -3s3;
    action nonDefAct() {
        if (var4 == 3s3) {
            meta.depth1 = var1 + var2;
        }
    }
    table stub {
        key = {
        }
        actions = {
            nonDefAct();
        }
        const default_action = nonDefAct();
        size = 1000000;
    }
    apply {
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

