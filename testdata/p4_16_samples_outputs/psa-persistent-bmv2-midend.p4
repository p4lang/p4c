#include <core.p4>
#include <bmv2/psa.p4>

struct EMPTY {
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header output_data_t {
    bit<32> word0;
    bit<32> word1;
    bit<32> word2;
    bit<32> word3;
}

struct headers_t {
    ethernet_t    ethernet;
    output_data_t output_data;
}

struct metadata_t {
}

parser MyIP(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in EMPTY resubmit_meta, in EMPTY recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

parser MyEP(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in EMPTY normal_meta, in EMPTY clone_i2e_meta, in EMPTY clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control MyIC(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    bit<8> hsiVar;
    bit<16> hsVar;
    @persistent @name("MyIC.reg") bit<16>[6] reg_0;
    @name("MyIC.orig_data") bit<16> orig_data_0;
    @name("MyIC.next_data") bit<16> next_data_0;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w1;
    }
    @hidden action psapersistentbmv2l78() {
        orig_data_0 = reg_0[8w0];
    }
    @hidden action psapersistentbmv2l78_0() {
        orig_data_0 = reg_0[8w1];
    }
    @hidden action psapersistentbmv2l78_1() {
        orig_data_0 = reg_0[8w2];
    }
    @hidden action psapersistentbmv2l78_2() {
        orig_data_0 = reg_0[8w3];
    }
    @hidden action psapersistentbmv2l78_3() {
        orig_data_0 = reg_0[8w4];
    }
    @hidden action psapersistentbmv2l78_4() {
        orig_data_0 = reg_0[8w5];
    }
    @hidden action psapersistentbmv2l78_5() {
        orig_data_0 = hsVar;
    }
    @hidden action psapersistentbmv2l78_6() {
        hsiVar = hdr.ethernet.dstAddr[7:0];
    }
    @hidden action psapersistentbmv2l82() {
        next_data_0 = hdr.ethernet.dstAddr[47:32];
    }
    @hidden action psapersistentbmv2l85() {
        next_data_0 = orig_data_0;
    }
    @hidden action psapersistentbmv2l88() {
        next_data_0 = orig_data_0 + 16w1;
    }
    @hidden action psapersistentbmv2l90() {
        orig_data_0 = 16w0xdead;
        next_data_0 = 16w0xbeef;
    }
    @hidden action psapersistentbmv2l94() {
        reg_0[8w0] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_0() {
        reg_0[8w1] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_1() {
        reg_0[8w2] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_2() {
        reg_0[8w3] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_3() {
        reg_0[8w4] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_4() {
        reg_0[8w5] = next_data_0;
    }
    @hidden action psapersistentbmv2l94_5() {
        hsiVar = hdr.ethernet.dstAddr[7:0];
    }
    @hidden action psapersistentbmv2l96() {
        hdr.output_data.word0 = (bit<32>)orig_data_0;
        hdr.output_data.word1 = (bit<32>)next_data_0;
    }
    @hidden table tbl_psapersistentbmv2l78 {
        actions = {
            psapersistentbmv2l78_6();
        }
        const default_action = psapersistentbmv2l78_6();
    }
    @hidden table tbl_psapersistentbmv2l78_0 {
        actions = {
            psapersistentbmv2l78();
        }
        const default_action = psapersistentbmv2l78();
    }
    @hidden table tbl_psapersistentbmv2l78_1 {
        actions = {
            psapersistentbmv2l78_0();
        }
        const default_action = psapersistentbmv2l78_0();
    }
    @hidden table tbl_psapersistentbmv2l78_2 {
        actions = {
            psapersistentbmv2l78_1();
        }
        const default_action = psapersistentbmv2l78_1();
    }
    @hidden table tbl_psapersistentbmv2l78_3 {
        actions = {
            psapersistentbmv2l78_2();
        }
        const default_action = psapersistentbmv2l78_2();
    }
    @hidden table tbl_psapersistentbmv2l78_4 {
        actions = {
            psapersistentbmv2l78_3();
        }
        const default_action = psapersistentbmv2l78_3();
    }
    @hidden table tbl_psapersistentbmv2l78_5 {
        actions = {
            psapersistentbmv2l78_4();
        }
        const default_action = psapersistentbmv2l78_4();
    }
    @hidden table tbl_psapersistentbmv2l78_6 {
        actions = {
            psapersistentbmv2l78_5();
        }
        const default_action = psapersistentbmv2l78_5();
    }
    @hidden table tbl_psapersistentbmv2l82 {
        actions = {
            psapersistentbmv2l82();
        }
        const default_action = psapersistentbmv2l82();
    }
    @hidden table tbl_psapersistentbmv2l85 {
        actions = {
            psapersistentbmv2l85();
        }
        const default_action = psapersistentbmv2l85();
    }
    @hidden table tbl_psapersistentbmv2l88 {
        actions = {
            psapersistentbmv2l88();
        }
        const default_action = psapersistentbmv2l88();
    }
    @hidden table tbl_psapersistentbmv2l90 {
        actions = {
            psapersistentbmv2l90();
        }
        const default_action = psapersistentbmv2l90();
    }
    @hidden table tbl_psapersistentbmv2l94 {
        actions = {
            psapersistentbmv2l94_5();
        }
        const default_action = psapersistentbmv2l94_5();
    }
    @hidden table tbl_psapersistentbmv2l94_0 {
        actions = {
            psapersistentbmv2l94();
        }
        const default_action = psapersistentbmv2l94();
    }
    @hidden table tbl_psapersistentbmv2l94_1 {
        actions = {
            psapersistentbmv2l94_0();
        }
        const default_action = psapersistentbmv2l94_0();
    }
    @hidden table tbl_psapersistentbmv2l94_2 {
        actions = {
            psapersistentbmv2l94_1();
        }
        const default_action = psapersistentbmv2l94_1();
    }
    @hidden table tbl_psapersistentbmv2l94_3 {
        actions = {
            psapersistentbmv2l94_2();
        }
        const default_action = psapersistentbmv2l94_2();
    }
    @hidden table tbl_psapersistentbmv2l94_4 {
        actions = {
            psapersistentbmv2l94_3();
        }
        const default_action = psapersistentbmv2l94_3();
    }
    @hidden table tbl_psapersistentbmv2l94_5 {
        actions = {
            psapersistentbmv2l94_4();
        }
        const default_action = psapersistentbmv2l94_4();
    }
    @hidden table tbl_psapersistentbmv2l96 {
        actions = {
            psapersistentbmv2l96();
        }
        const default_action = psapersistentbmv2l96();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        if (hdr.ethernet.isValid()) {
            if (hdr.ethernet.dstAddr[15:8] >= 8w1 && hdr.ethernet.dstAddr[15:8] <= 8w3) {
                tbl_psapersistentbmv2l78.apply();
                if (hsiVar == 8w0) {
                    tbl_psapersistentbmv2l78_0.apply();
                } else if (hsiVar == 8w1) {
                    tbl_psapersistentbmv2l78_1.apply();
                } else if (hsiVar == 8w2) {
                    tbl_psapersistentbmv2l78_2.apply();
                } else if (hsiVar == 8w3) {
                    tbl_psapersistentbmv2l78_3.apply();
                } else if (hsiVar == 8w4) {
                    tbl_psapersistentbmv2l78_4.apply();
                } else if (hsiVar == 8w5) {
                    tbl_psapersistentbmv2l78_5.apply();
                } else if (hsiVar >= 8w5) {
                    tbl_psapersistentbmv2l78_6.apply();
                }
            }
            if (hdr.ethernet.dstAddr[15:8] == 8w1) {
                tbl_psapersistentbmv2l82.apply();
            } else if (hdr.ethernet.dstAddr[15:8] == 8w2) {
                tbl_psapersistentbmv2l85.apply();
            } else if (hdr.ethernet.dstAddr[15:8] == 8w3) {
                tbl_psapersistentbmv2l88.apply();
            } else {
                tbl_psapersistentbmv2l90.apply();
            }
            if (hdr.ethernet.dstAddr[7:0] < 8w6) {
                tbl_psapersistentbmv2l94.apply();
                if (hsiVar == 8w0) {
                    tbl_psapersistentbmv2l94_0.apply();
                } else if (hsiVar == 8w1) {
                    tbl_psapersistentbmv2l94_1.apply();
                } else if (hsiVar == 8w2) {
                    tbl_psapersistentbmv2l94_2.apply();
                } else if (hsiVar == 8w3) {
                    tbl_psapersistentbmv2l94_3.apply();
                } else if (hsiVar == 8w4) {
                    tbl_psapersistentbmv2l94_4.apply();
                } else if (hsiVar == 8w5) {
                    tbl_psapersistentbmv2l94_5.apply();
                }
            }
            tbl_psapersistentbmv2l96.apply();
        }
        tbl_send_to_port.apply();
    }
}

control MyEC(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control MyID(packet_out pkt, out EMPTY clone_i2e_meta, out EMPTY resubmit_meta, out EMPTY normal_meta, inout headers_t hdr, in metadata_t user_meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psapersistentbmv2l122() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psapersistentbmv2l122 {
        actions = {
            psapersistentbmv2l122();
        }
        const default_action = psapersistentbmv2l122();
    }
    apply {
        tbl_psapersistentbmv2l122.apply();
    }
}

control MyED(packet_out pkt, out EMPTY clone_e2e_meta, out EMPTY recirculate_meta, inout headers_t hdr, in metadata_t user_meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
