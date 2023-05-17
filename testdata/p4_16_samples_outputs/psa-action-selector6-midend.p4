#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

struct user_meta_t {
    bit<16> data;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t ethernet;
}

parser MyIP(packet_in buffer, out headers_t hdr, inout user_meta_t b, in psa_ingress_parser_input_metadata_t c, in EMPTY d, in EMPTY e) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

parser MyEP(packet_in buffer, out EMPTY a, inout EMPTY b, in psa_egress_parser_input_metadata_t c, in EMPTY d, in EMPTY e, in EMPTY f) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout user_meta_t b, in psa_ingress_input_metadata_t c, inout psa_ingress_output_metadata_t d) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_4() {
    }
    @name("MyIC.ap") ActionProfile(32w1024) ap_0;
    @name("MyIC.as") ActionSelector(PSA_HashAlgorithm_t.CRC32, 32w1024, 32w16) as_0;
    @name(".a1") action a1(@name("param") bit<48> param) {
        hdr.ethernet.dstAddr = param;
    }
    @name(".a1") action a1_1(@name("param") bit<48> param_2) {
        hdr.ethernet.dstAddr = param_2;
    }
    @name(".a2") action a2(@name("param") bit<16> param_3) {
        hdr.ethernet.etherType = param_3;
    }
    @name(".a2") action a2_1(@name("param") bit<16> param_4) {
        hdr.ethernet.etherType = param_4;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
            b.data              : selector @name("b.data");
        }
        actions = {
            NoAction_1();
            a1();
            a2();
        }
        psa_implementation = as_0;
        default_action = NoAction_1();
    }
    @name("MyIC.tbl1") table tbl1_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_2();
            a1_1();
            a2_1();
        }
        psa_implementation = ap_0;
        default_action = NoAction_2();
    }
    @name("MyIC.foo") table foo_0 {
        actions = {
            NoAction_3();
        }
        default_action = NoAction_3();
    }
    @name("MyIC.bar") table bar_0 {
        actions = {
            NoAction_4();
        }
        default_action = NoAction_4();
    }
    apply {
        switch (tbl_0.apply().action_run) {
            a1: {
                foo_0.apply();
            }
            a2: {
                bar_0.apply();
            }
            default: {
            }
        }
        switch (tbl_0.apply().action_run) {
            a1: {
                bar_0.apply();
            }
            a2: {
                foo_0.apply();
            }
            default: {
            }
        }
        switch (tbl1_0.apply().action_run) {
            a1_1: {
                bar_0.apply();
            }
            a2_1: {
                foo_0.apply();
            }
            default: {
                foo_0.apply();
                bar_0.apply();
            }
        }
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_t e, in psa_ingress_output_metadata_t f) {
    @hidden action psaactionselector6l117() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaactionselector6l117 {
        actions = {
            psaactionselector6l117();
        }
        const default_action = psaactionselector6l117();
    }
    apply {
        tbl_psaactionselector6l117.apply();
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
