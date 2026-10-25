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
    @hidden action psaendofingresstestbmv2l91() {
        ostd.resubmit = (bool)hdr.ethernet.dstAddr[32:32];
    }
    @hidden action psaendofingresstestbmv2l87() {
        ostd.drop = (bool)hdr.ethernet.dstAddr[40:40];
    }
    @hidden action psaendofingresstestbmv2l47() {
        hdr.output_data.word0 = 32w1;
    }
    @hidden action psaendofingresstestbmv2l49() {
        hdr.output_data.word0 = 32w2;
    }
    @hidden action psaendofingresstestbmv2l51() {
        hdr.output_data.word0 = 32w3;
    }
    @hidden action psaendofingresstestbmv2l53() {
        hdr.output_data.word0 = 32w4;
    }
    @hidden action psaendofingresstestbmv2l55() {
        hdr.output_data.word0 = 32w5;
    }
    @hidden action psaendofingresstestbmv2l57() {
        hdr.output_data.word0 = 32w6;
    }
    @hidden action psaendofingresstestbmv2l59() {
        hdr.output_data.word0 = 32w7;
    }
    @hidden action psaendofingresstestbmv2l45() {
        ostd.multicast_group = (bit<32>)hdr.ethernet.dstAddr[31:16];
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr[15:0];
        hdr.output_data.word0 = 32w8;
    }
    @hidden table tbl_psaendofingresstestbmv2l87 {
        actions = {
            psaendofingresstestbmv2l87();
        }
        const default_action = psaendofingresstestbmv2l87();
    }
    @hidden table tbl_psaendofingresstestbmv2l91 {
        actions = {
            psaendofingresstestbmv2l91();
        }
        const default_action = psaendofingresstestbmv2l91();
    }
    @hidden table tbl_psaendofingresstestbmv2l45 {
        actions = {
            psaendofingresstestbmv2l45();
        }
        const default_action = psaendofingresstestbmv2l45();
    }
    @hidden table tbl_psaendofingresstestbmv2l47 {
        actions = {
            psaendofingresstestbmv2l47();
        }
        const default_action = psaendofingresstestbmv2l47();
    }
    @hidden table tbl_psaendofingresstestbmv2l49 {
        actions = {
            psaendofingresstestbmv2l49();
        }
        const default_action = psaendofingresstestbmv2l49();
    }
    @hidden table tbl_psaendofingresstestbmv2l51 {
        actions = {
            psaendofingresstestbmv2l51();
        }
        const default_action = psaendofingresstestbmv2l51();
    }
    @hidden table tbl_psaendofingresstestbmv2l53 {
        actions = {
            psaendofingresstestbmv2l53();
        }
        const default_action = psaendofingresstestbmv2l53();
    }
    @hidden table tbl_psaendofingresstestbmv2l55 {
        actions = {
            psaendofingresstestbmv2l55();
        }
        const default_action = psaendofingresstestbmv2l55();
    }
    @hidden table tbl_psaendofingresstestbmv2l57 {
        actions = {
            psaendofingresstestbmv2l57();
        }
        const default_action = psaendofingresstestbmv2l57();
    }
    @hidden table tbl_psaendofingresstestbmv2l59 {
        actions = {
            psaendofingresstestbmv2l59();
        }
        const default_action = psaendofingresstestbmv2l59();
    }
    apply {
        tbl_psaendofingresstestbmv2l87.apply();
        if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            ;
        } else {
            tbl_psaendofingresstestbmv2l91.apply();
        }
        tbl_psaendofingresstestbmv2l45.apply();
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            tbl_psaendofingresstestbmv2l47.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            tbl_psaendofingresstestbmv2l49.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            tbl_psaendofingresstestbmv2l51.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psaendofingresstestbmv2l53.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psaendofingresstestbmv2l55.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            tbl_psaendofingresstestbmv2l57.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psaendofingresstestbmv2l59.apply();
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
    @hidden action psaendofingresstestbmv2l129() {
        hdr.output_data.word0 = (bit<32>)egress_pkt_seen_0.read(hdr.ethernet.etherType[7:0]);
    }
    @hidden action psaendofingresstestbmv2l133() {
        egress_pkt_seen_0.write(hdr.ethernet.etherType[7:0], hdr.ethernet.srcAddr[15:0]);
    }
    @hidden action psaendofingresstestbmv2l154() {
        egress_pkt_seen_0.write(hdr.ethernet.etherType[7:0], 16w1);
    }
    @hidden action psaendofingresstestbmv2l47_0() {
        hdr.output_data.word3 = 32w1;
    }
    @hidden action psaendofingresstestbmv2l49_0() {
        hdr.output_data.word3 = 32w2;
    }
    @hidden action psaendofingresstestbmv2l51_0() {
        hdr.output_data.word3 = 32w3;
    }
    @hidden action psaendofingresstestbmv2l53_0() {
        hdr.output_data.word3 = 32w4;
    }
    @hidden action psaendofingresstestbmv2l55_0() {
        hdr.output_data.word3 = 32w5;
    }
    @hidden action psaendofingresstestbmv2l57_0() {
        hdr.output_data.word3 = 32w6;
    }
    @hidden action psaendofingresstestbmv2l59_0() {
        hdr.output_data.word3 = 32w7;
    }
    @hidden action psaendofingresstestbmv2l45_0() {
        hdr.output_data.word1 = (bit<32>)istd.egress_port;
        hdr.output_data.word2 = (bit<32>)(bit<16>)istd.instance;
        hdr.output_data.word3 = 32w8;
    }
    @hidden table tbl_psaendofingresstestbmv2l129 {
        actions = {
            psaendofingresstestbmv2l129();
        }
        const default_action = psaendofingresstestbmv2l129();
    }
    @hidden table tbl_psaendofingresstestbmv2l133 {
        actions = {
            psaendofingresstestbmv2l133();
        }
        const default_action = psaendofingresstestbmv2l133();
    }
    @hidden table tbl_psaendofingresstestbmv2l154 {
        actions = {
            psaendofingresstestbmv2l154();
        }
        const default_action = psaendofingresstestbmv2l154();
    }
    @hidden table tbl_psaendofingresstestbmv2l45_0 {
        actions = {
            psaendofingresstestbmv2l45_0();
        }
        const default_action = psaendofingresstestbmv2l45_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l47_0 {
        actions = {
            psaendofingresstestbmv2l47_0();
        }
        const default_action = psaendofingresstestbmv2l47_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l49_0 {
        actions = {
            psaendofingresstestbmv2l49_0();
        }
        const default_action = psaendofingresstestbmv2l49_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l51_0 {
        actions = {
            psaendofingresstestbmv2l51_0();
        }
        const default_action = psaendofingresstestbmv2l51_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l53_0 {
        actions = {
            psaendofingresstestbmv2l53_0();
        }
        const default_action = psaendofingresstestbmv2l53_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l55_0 {
        actions = {
            psaendofingresstestbmv2l55_0();
        }
        const default_action = psaendofingresstestbmv2l55_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l57_0 {
        actions = {
            psaendofingresstestbmv2l57_0();
        }
        const default_action = psaendofingresstestbmv2l57_0();
    }
    @hidden table tbl_psaendofingresstestbmv2l59_0 {
        actions = {
            psaendofingresstestbmv2l59_0();
        }
        const default_action = psaendofingresstestbmv2l59_0();
    }
    apply {
        if (hdr.ethernet.etherType[15:8] == 8w0xc0) {
            tbl_psaendofingresstestbmv2l129.apply();
        } else if (hdr.ethernet.etherType[15:8] == 8w0xc1) {
            tbl_psaendofingresstestbmv2l133.apply();
        } else {
            if (hdr.ethernet.etherType < 16w256) {
                tbl_psaendofingresstestbmv2l154.apply();
            }
            tbl_psaendofingresstestbmv2l45_0.apply();
            if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
                tbl_psaendofingresstestbmv2l47_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
                tbl_psaendofingresstestbmv2l49_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
                tbl_psaendofingresstestbmv2l51_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
                tbl_psaendofingresstestbmv2l53_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
                tbl_psaendofingresstestbmv2l55_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
                tbl_psaendofingresstestbmv2l57_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
                tbl_psaendofingresstestbmv2l59_0.apply();
            }
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaendofingresstestbmv2l167() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaendofingresstestbmv2l167 {
        actions = {
            psaendofingresstestbmv2l167();
        }
        const default_action = psaendofingresstestbmv2l167();
    }
    apply {
        tbl_psaendofingresstestbmv2l167.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaendofingresstestbmv2l167_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psaendofingresstestbmv2l167_0 {
        actions = {
            psaendofingresstestbmv2l167_0();
        }
        const default_action = psaendofingresstestbmv2l167_0();
    }
    apply {
        tbl_psaendofingresstestbmv2l167_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
