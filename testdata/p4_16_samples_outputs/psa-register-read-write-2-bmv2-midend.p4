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
    @name("MyIC.orig_data") bit<16> orig_data_0;
    @name("MyIC.next_data") bit<16> next_data_0;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w1;
    }
    @name("MyIC.reg") Register<bit<16>, bit<8>>(32w6) reg_0;
    @hidden action psaregisterreadwrite2bmv2l78() {
        orig_data_0 = reg_0.read(hdr.ethernet.dstAddr[7:0]);
    }
    @hidden action psaregisterreadwrite2bmv2l82() {
        next_data_0 = hdr.ethernet.dstAddr[47:32];
    }
    @hidden action psaregisterreadwrite2bmv2l85() {
        next_data_0 = orig_data_0;
    }
    @hidden action psaregisterreadwrite2bmv2l88() {
        next_data_0 = orig_data_0 + 16w1;
    }
    @hidden action psaregisterreadwrite2bmv2l90() {
        orig_data_0 = 16w0xdead;
        next_data_0 = 16w0xbeef;
    }
    @hidden action psaregisterreadwrite2bmv2l93() {
        reg_0.write(hdr.ethernet.dstAddr[7:0], next_data_0);
        hdr.output_data.word0 = (bit<32>)orig_data_0;
        hdr.output_data.word1 = (bit<32>)next_data_0;
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l78 {
        actions = {
            psaregisterreadwrite2bmv2l78();
        }
        const default_action = psaregisterreadwrite2bmv2l78();
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l82 {
        actions = {
            psaregisterreadwrite2bmv2l82();
        }
        const default_action = psaregisterreadwrite2bmv2l82();
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l85 {
        actions = {
            psaregisterreadwrite2bmv2l85();
        }
        const default_action = psaregisterreadwrite2bmv2l85();
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l88 {
        actions = {
            psaregisterreadwrite2bmv2l88();
        }
        const default_action = psaregisterreadwrite2bmv2l88();
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l90 {
        actions = {
            psaregisterreadwrite2bmv2l90();
        }
        const default_action = psaregisterreadwrite2bmv2l90();
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l93 {
        actions = {
            psaregisterreadwrite2bmv2l93();
        }
        const default_action = psaregisterreadwrite2bmv2l93();
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
                tbl_psaregisterreadwrite2bmv2l78.apply();
            }
            if (hdr.ethernet.dstAddr[15:8] == 8w1) {
                tbl_psaregisterreadwrite2bmv2l82.apply();
            } else if (hdr.ethernet.dstAddr[15:8] == 8w2) {
                tbl_psaregisterreadwrite2bmv2l85.apply();
            } else if (hdr.ethernet.dstAddr[15:8] == 8w3) {
                tbl_psaregisterreadwrite2bmv2l88.apply();
            } else {
                tbl_psaregisterreadwrite2bmv2l90.apply();
            }
            tbl_psaregisterreadwrite2bmv2l93.apply();
        }
        tbl_send_to_port.apply();
    }
}

control MyEC(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control MyID(packet_out pkt, out EMPTY clone_i2e_meta, out EMPTY resubmit_meta, out EMPTY normal_meta, inout headers_t hdr, in metadata_t user_meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaregisterreadwrite2bmv2l121() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaregisterreadwrite2bmv2l121 {
        actions = {
            psaregisterreadwrite2bmv2l121();
        }
        const default_action = psaregisterreadwrite2bmv2l121();
    }
    apply {
        tbl_psaregisterreadwrite2bmv2l121.apply();
    }
}

control MyED(packet_out pkt, out EMPTY clone_e2e_meta, out EMPTY recirculate_meta, inout headers_t hdr, in metadata_t user_meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
    }
}

IngressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyIP(), MyIC(), MyID()) ip;
EgressPipeline<headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY>(MyEP(), MyEC(), MyED()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
