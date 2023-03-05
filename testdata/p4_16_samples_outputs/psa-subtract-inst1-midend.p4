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
    @name("MyIngressControl.var1") bit<5> var1_0;
    @name("MyIngressControl.var2") bit<5> var2_0;
    @name("MyIngressControl.var3") int<33> var3_0;
    @name("MyIngressControl.var4") int<33> var4_0;
    @name("MyIngressControl.var5") int<33> var5_0;
    @name("MyIngressControl.nonDefAct") action nonDefAct() {
        if (var4_0 == 33s3 && var5_0 == 33s1) {
            meta.depth3 = meta.depth3 + 5w29;
            meta.depth4 = var1_0 + var2_0;
            meta.depth5 = var1_0 ^ 5w2;
            meta.depth6 = var2_0 + 5w31;
            meta.depth7 = var2_0 + 5w3;
        }
    }
    @name("MyIngressControl.stub") table stub_0 {
        actions = {
            nonDefAct();
        }
        const default_action = nonDefAct();
        size = 1000000;
    }
    @hidden action psasubtractinst1l62() {
        var1_0 = 5w8;
        var2_0 = 5w2;
        var4_0 = var3_0 + -33s3;
        var5_0 = var3_0 + -33s3 - var3_0;
        meta.depth4 = meta.depth2 + 5w28;
        d.egress_port = (bit<32>)c.ingress_port ^ 32w1;
    }
    @hidden table tbl_psasubtractinst1l62 {
        actions = {
            psasubtractinst1l62();
        }
        const default_action = psasubtractinst1l62();
    }
    apply {
        tbl_psasubtractinst1l62.apply();
        stub_0.apply();
    }
}

control MyIngressDeparser(packet_out pkt, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_data_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psasubtractinst1l104() {
        pkt.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psasubtractinst1l104 {
        actions = {
            psasubtractinst1l104();
        }
        const default_action = psasubtractinst1l104();
    }
    apply {
        tbl_psasubtractinst1l104.apply();
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
