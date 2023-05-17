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
    @name("MyIC.tmp") bit<16> tmp_0;
    @name("MyIC.tmp_1") bit<16> tmp_1;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("MyIC.execute") action execute_1() {
        if (b.data != 16w0) {
            tmp_0 = 16w0;
        } else {
            tmp_0 = 16w1;
        }
        b.data = tmp_0 + 16w1;
    }
    @name("MyIC.tbl") table tbl_0 {
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr");
        }
        actions = {
            NoAction_1();
            execute_1();
        }
        default_action = NoAction_1();
    }
    @hidden action psaconditional_operator67() {
        tmp_1 = 16w2;
    }
    @hidden action psaconditional_operator67_0() {
        tmp_1 = 16w5;
    }
    @hidden action psaconditional_operator68() {
        b.data = tmp_1 + 16w5;
    }
    @hidden table tbl_psaconditional_operator67 {
        actions = {
            psaconditional_operator67();
        }
        const default_action = psaconditional_operator67();
    }
    @hidden table tbl_psaconditional_operator67_0 {
        actions = {
            psaconditional_operator67_0();
        }
        const default_action = psaconditional_operator67_0();
    }
    @hidden table tbl_psaconditional_operator68 {
        actions = {
            psaconditional_operator68();
        }
        const default_action = psaconditional_operator68();
    }
    apply {
        if (b.data != 16w0) {
            tbl_psaconditional_operator67.apply();
        } else {
            tbl_psaconditional_operator67_0.apply();
        }
        tbl_psaconditional_operator68.apply();
        tbl_0.apply();
    }
}

control MyEC(inout EMPTY a, inout EMPTY b, in psa_egress_input_metadata_t c, inout psa_egress_output_metadata_t d) {
    apply {
    }
}

control MyID(packet_out buffer, out EMPTY a, out EMPTY b, out EMPTY c, inout headers_t hdr, in user_meta_t e, in psa_ingress_output_metadata_t f) {
    apply {
    }
}

control MyED(packet_out buffer, out EMPTY a, out EMPTY b, inout EMPTY c, in EMPTY d, in psa_egress_output_metadata_t e, in psa_egress_deparser_input_metadata_t f) {
    apply {
    }
}

IngressPipeline<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, user_meta_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
