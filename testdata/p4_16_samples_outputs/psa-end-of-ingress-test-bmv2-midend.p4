#include <core.p4>
#include <bmv2/psa.p4>

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

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t    ethernet;
    output_data_t output_data;
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @hidden action psaendofingresstestbmv2l100() {
        ostd.resubmit = (bool)hdr.ethernet.dstAddr[32:32];
    }
    @hidden action psaendofingresstestbmv2l96() {
        ostd.drop = (bool)hdr.ethernet.dstAddr[40:40];
    }
    @hidden action psaendofingresstestbmv2l56() {
        hdr.output_data.word0 = 32w1;
    }
    @hidden action psaendofingresstestbmv2l58() {
        hdr.output_data.word0 = 32w2;
    }
    @hidden action psaendofingresstestbmv2l60() {
        hdr.output_data.word0 = 32w3;
    }
    @hidden action psaendofingresstestbmv2l62() {
        hdr.output_data.word0 = 32w4;
    }
    @hidden action psaendofingresstestbmv2l64() {
        hdr.output_data.word0 = 32w5;
    }
    @hidden action psaendofingresstestbmv2l66() {
        hdr.output_data.word0 = 32w6;
    }
    @hidden action psaendofingresstestbmv2l68() {
        hdr.output_data.word0 = 32w7;
    }
    @hidden action psaendofingresstestbmv2l54() {
        ostd.multicast_group = (bit<32>)hdr.ethernet.dstAddr[31:16];
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr[15:0];
        hdr.output_data.word0 = 32w8;
    }
    @hidden table tbl_psaendofingresstestbmv2l96 {
        actions = {
            psaendofingresstestbmv2l96();
        }
        const default_action = psaendofingresstestbmv2l96();
    }
    @hidden table tbl_psaendofingresstestbmv2l100 {
        actions = {
            psaendofingresstestbmv2l100();
        }
        const default_action = psaendofingresstestbmv2l100();
    }
    @hidden table tbl_psaendofingresstestbmv2l54 {
        actions = {
            psaendofingresstestbmv2l54();
        }
        const default_action = psaendofingresstestbmv2l54();
    }
    @hidden table tbl_psaendofingresstestbmv2l56 {
        actions = {
            psaendofingresstestbmv2l56();
        }
        const default_action = psaendofingresstestbmv2l56();
    }
    @hidden table tbl_psaendofingresstestbmv2l58 {
        actions = {
            psaendofingresstestbmv2l58();
        }
        const default_action = psaendofingresstestbmv2l58();
    }
    @hidden table tbl_psaendofingresstestbmv2l60 {
        actions = {
            psaendofingresstestbmv2l60();
        }
        const default_action = psaendofingresstestbmv2l60();
    }
    @hidden table tbl_psaendofingresstestbmv2l62 {
        actions = {
            psaendofingresstestbmv2l62();
        }
        const default_action = psaendofingresstestbmv2l62();
    }
    @hidden table tbl_psaendofingresstestbmv2l64 {
        actions = {
            psaendofingresstestbmv2l64();
        }
        const default_action = psaendofingresstestbmv2l64();
    }
    @hidden table tbl_psaendofingresstestbmv2l66 {
        actions = {
            psaendofingresstestbmv2l66();
        }
        const default_action = psaendofingresstestbmv2l66();
    }
    @hidden table tbl_psaendofingresstestbmv2l68 {
        actions = {
            psaendofingresstestbmv2l68();
        }
        const default_action = psaendofingresstestbmv2l68();
    }
    apply {
        tbl_psaendofingresstestbmv2l96.apply();
        if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            ;
        } else {
            tbl_psaendofingresstestbmv2l100.apply();
        }
        tbl_psaendofingresstestbmv2l54.apply();
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            tbl_psaendofingresstestbmv2l56.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            tbl_psaendofingresstestbmv2l58.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            tbl_psaendofingresstestbmv2l60.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psaendofingresstestbmv2l62.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psaendofingresstestbmv2l64.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            tbl_psaendofingresstestbmv2l66.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psaendofingresstestbmv2l68.apply();
        }
    }
}

parser EgressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    @name("cEgress.egress_pkt_seen") Register<bit<16>, bit<8>>(32w256) egress_pkt_seen_0;
    @hidden action psaendofingresstestbmv2l138() {
        hdr.output_data.word0 = (bit<32>)egress_pkt_seen_0.read(hdr.ethernet.etherType[7:0]);
    }
    @hidden action psaendofingresstestbmv2l142() {
        egress_pkt_seen_0.write(hdr.ethernet.etherType[7:0], hdr.ethernet.srcAddr[15:0]);
    }
    @hidden action psaendofingresstestbmv2l163() {
        egress_pkt_seen_0.write(hdr.ethernet.etherType[7:0], 16w1);
    }
    @hidden action psaendofingresstestbmv2l56_0() {
        hdr.output_data.word3 = 32w1;
    }
    @hidden action psaendofingresstestbmv2l58_0() {
        hdr.output_data.word3 = 32w2;
    }
    @hidden action psaendofingresstestbmv2l60_0() {
        hdr.output_data.word3 = 32w3;
    }
    @hidden action psaendofingresstestbmv2l62_0() {
        hdr.output_data.word3 = 32w4;
    }
    @hidden action psaendofingresstestbmv2l64_0() {
        hdr.output_data.word3 = 32w5;
    }
    @hidden action psaendofingresstestbmv2l66_0() {
        hdr.output_data.word3 = 32w6;
    }
    @hidden action psaendofingresstestbmv2l68_0() {
        hdr.output_data.word3 = 32w7;
    }
    @hidden action psaendofingresstestbmv2l54_0() {
        hdr.output_data.word1 = (bit<32>)istd.egress_port;
        hdr.output_data.word2 = (bit<32>)(bit<16>)istd.instance;
        hdr.output_data.word3 = 32w8;
    }
    @hidden table tbl_psaendofingresstestbmv2l138 {
        actions = {
            psaendofingresstestbmv2l138();
        }
        const default_action = psaendofingresstestbmv2l138();
    }
    @hidden table tbl_psaendofingresstestbmv2l142 {
        actions = {
            psaendofingresstestbmv2l142();
        }
        const default_action = psaendofingresstestbmv2l142();
    }
    @hidden table tbl_psaendofingresstestbmv2l163 {
        actions = {
            psaendofingresstestbmv2l163();
        }
        const default_action = psaendofingresstestbmv2l163();
    }
    @hidden table tbl_psaendofingresstestbmv2l54_0 {
        actions = {
            psaendofingresstestbmv2l54_0();
        }
        const default_action = psaendofingresstestbmv2l54_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l56_0 {
        actions = {
            psaendofingresstestbmv2l56_0();
        }
        const default_action = psaendofingresstestbmv2l56_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l58_0 {
        actions = {
            psaendofingresstestbmv2l58_0();
        }
        const default_action = psaendofingresstestbmv2l58_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l60_0 {
        actions = {
            psaendofingresstestbmv2l60_0();
        }
        const default_action = psaendofingresstestbmv2l60_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l62_0 {
        actions = {
            psaendofingresstestbmv2l62_0();
        }
        const default_action = psaendofingresstestbmv2l62_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l64_0 {
        actions = {
            psaendofingresstestbmv2l64_0();
        }
        const default_action = psaendofingresstestbmv2l64_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l66_0 {
        actions = {
            psaendofingresstestbmv2l66_0();
        }
        const default_action = psaendofingresstestbmv2l66_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l68_0 {
        actions = {
            psaendofingresstestbmv2l68_0();
        }
        const default_action = psaendofingresstestbmv2l68_0();
    }
    apply {
        if (hdr.ethernet.etherType[15:8] == 8w0xc0) {
            tbl_psaendofingresstestbmv2l138.apply();
        } else if (hdr.ethernet.etherType[15:8] == 8w0xc1) {
            tbl_psaendofingresstestbmv2l142.apply();
        } else {
            if (hdr.ethernet.etherType < 16w256) {
                tbl_psaendofingresstestbmv2l163.apply();
            }
            tbl_psaendofingresstestbmv2l54_0.apply();
            if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
                tbl_psaendofingresstestbmv2l56_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
                tbl_psaendofingresstestbmv2l58_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
                tbl_psaendofingresstestbmv2l60_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
                tbl_psaendofingresstestbmv2l62_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
                tbl_psaendofingresstestbmv2l64_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
                tbl_psaendofingresstestbmv2l66_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
                tbl_psaendofingresstestbmv2l68_0.apply();
            }
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaendofingresstestbmv2l176() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaendofingresstestbmv2l176 {
        actions = {
            psaendofingresstestbmv2l176();
        }
        const default_action = psaendofingresstestbmv2l176();
    }
    apply {
        tbl_psaendofingresstestbmv2l176.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaendofingresstestbmv2l176_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaendofingresstestbmv2l176_0 {
        actions = {
            psaendofingresstestbmv2l176_0();
        }
        const default_action = psaendofingresstestbmv2l176_0();
    }
    apply {
        tbl_psaendofingresstestbmv2l176_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
