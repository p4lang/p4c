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
    @name("cIngress.int_packet_path") bit<32> int_packet_path_0;
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = 32w0xfffffffa;
    }
    @name("cIngress.record_ingress_ports_in_pkt") action record_ingress_ports_in_pkt() {
        hdr.output_data.word1 = (bit<32>)istd.ingress_port;
    }
    @hidden action psarecirculatenometabmv2l56() {
        int_packet_path_0 = 32w1;
    }
    @hidden action psarecirculatenometabmv2l58() {
        int_packet_path_0 = 32w2;
    }
    @hidden action psarecirculatenometabmv2l60() {
        int_packet_path_0 = 32w3;
    }
    @hidden action psarecirculatenometabmv2l62() {
        int_packet_path_0 = 32w4;
    }
    @hidden action psarecirculatenometabmv2l64() {
        int_packet_path_0 = 32w5;
    }
    @hidden action psarecirculatenometabmv2l66() {
        int_packet_path_0 = 32w6;
    }
    @hidden action psarecirculatenometabmv2l68() {
        int_packet_path_0 = 32w7;
    }
    @hidden action psarecirculatenometabmv2l54() {
        int_packet_path_0 = 32w8;
    }
    @hidden action psarecirculatenometabmv2l110() {
        hdr.output_data.word2 = int_packet_path_0;
    }
    @hidden action psarecirculatenometabmv2l115() {
        hdr.output_data.word0 = int_packet_path_0;
    }
    @hidden table tbl_record_ingress_ports_in_pkt {
        actions = {
            record_ingress_ports_in_pkt();
        }
        const default_action = record_ingress_ports_in_pkt();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_1();
        }
        const default_action = send_to_port_1();
    }
    @hidden table tbl_send_to_port_0 {
        actions = {
            send_to_port_2();
        }
        const default_action = send_to_port_2();
    }
    @hidden table tbl_psarecirculatenometabmv2l54 {
        actions = {
            psarecirculatenometabmv2l54();
        }
        const default_action = psarecirculatenometabmv2l54();
    }
    @hidden table tbl_psarecirculatenometabmv2l56 {
        actions = {
            psarecirculatenometabmv2l56();
        }
        const default_action = psarecirculatenometabmv2l56();
    }
    @hidden table tbl_psarecirculatenometabmv2l58 {
        actions = {
            psarecirculatenometabmv2l58();
        }
        const default_action = psarecirculatenometabmv2l58();
    }
    @hidden table tbl_psarecirculatenometabmv2l60 {
        actions = {
            psarecirculatenometabmv2l60();
        }
        const default_action = psarecirculatenometabmv2l60();
    }
    @hidden table tbl_psarecirculatenometabmv2l62 {
        actions = {
            psarecirculatenometabmv2l62();
        }
        const default_action = psarecirculatenometabmv2l62();
    }
    @hidden table tbl_psarecirculatenometabmv2l64 {
        actions = {
            psarecirculatenometabmv2l64();
        }
        const default_action = psarecirculatenometabmv2l64();
    }
    @hidden table tbl_psarecirculatenometabmv2l66 {
        actions = {
            psarecirculatenometabmv2l66();
        }
        const default_action = psarecirculatenometabmv2l66();
    }
    @hidden table tbl_psarecirculatenometabmv2l68 {
        actions = {
            psarecirculatenometabmv2l68();
        }
        const default_action = psarecirculatenometabmv2l68();
    }
    @hidden table tbl_psarecirculatenometabmv2l110 {
        actions = {
            psarecirculatenometabmv2l110();
        }
        const default_action = psarecirculatenometabmv2l110();
    }
    @hidden table tbl_psarecirculatenometabmv2l115 {
        actions = {
            psarecirculatenometabmv2l115();
        }
        const default_action = psarecirculatenometabmv2l115();
    }
    apply {
        if (hdr.ethernet.dstAddr[3:0] >= 4w4) {
            tbl_record_ingress_ports_in_pkt.apply();
            tbl_send_to_port.apply();
        } else {
            tbl_send_to_port_0.apply();
        }
        tbl_psarecirculatenometabmv2l54.apply();
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            tbl_psarecirculatenometabmv2l56.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            tbl_psarecirculatenometabmv2l58.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            tbl_psarecirculatenometabmv2l60.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            tbl_psarecirculatenometabmv2l62.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            tbl_psarecirculatenometabmv2l64.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            tbl_psarecirculatenometabmv2l66.apply();
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psarecirculatenometabmv2l68.apply();
        }
        if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            tbl_psarecirculatenometabmv2l110.apply();
        } else {
            tbl_psarecirculatenometabmv2l115.apply();
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
    @name("cEgress.add") action add_1() {
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr + hdr.ethernet.srcAddr;
    }
    @name("cEgress.e") table e_0 {
        actions = {
            add_1();
        }
        default_action = add_1();
    }
    @hidden action psarecirculatenometabmv2l56_0() {
        hdr.output_data.word3 = 32w1;
    }
    @hidden action psarecirculatenometabmv2l58_0() {
        hdr.output_data.word3 = 32w2;
    }
    @hidden action psarecirculatenometabmv2l60_0() {
        hdr.output_data.word3 = 32w3;
    }
    @hidden action psarecirculatenometabmv2l62_0() {
        hdr.output_data.word3 = 32w4;
    }
    @hidden action psarecirculatenometabmv2l64_0() {
        hdr.output_data.word3 = 32w5;
    }
    @hidden action psarecirculatenometabmv2l66_0() {
        hdr.output_data.word3 = 32w6;
    }
    @hidden action psarecirculatenometabmv2l68_0() {
        hdr.output_data.word3 = 32w7;
    }
    @hidden action psarecirculatenometabmv2l54_0() {
        hdr.output_data.word3 = 32w8;
    }
    @hidden table tbl_psarecirculatenometabmv2l54_0 {
        actions = {
            psarecirculatenometabmv2l54_0();
        }
        const default_action = psarecirculatenometabmv2l54_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l56_0 {
        actions = {
            psarecirculatenometabmv2l56_0();
        }
        const default_action = psarecirculatenometabmv2l56_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l58_0 {
        actions = {
            psarecirculatenometabmv2l58_0();
        }
        const default_action = psarecirculatenometabmv2l58_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l60_0 {
        actions = {
            psarecirculatenometabmv2l60_0();
        }
        const default_action = psarecirculatenometabmv2l60_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l62_0 {
        actions = {
            psarecirculatenometabmv2l62_0();
        }
        const default_action = psarecirculatenometabmv2l62_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l64_0 {
        actions = {
            psarecirculatenometabmv2l64_0();
        }
        const default_action = psarecirculatenometabmv2l64_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l66_0 {
        actions = {
            psarecirculatenometabmv2l66_0();
        }
        const default_action = psarecirculatenometabmv2l66_0();
    }
    @hidden table tbl_psarecirculatenometabmv2l68_0 {
        actions = {
            psarecirculatenometabmv2l68_0();
        }
        const default_action = psarecirculatenometabmv2l68_0();
    }
    apply {
        e_0.apply();
        if (istd.egress_port == 32w0xfffffffa) {
            tbl_psarecirculatenometabmv2l54_0.apply();
            if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
                tbl_psarecirculatenometabmv2l56_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
                tbl_psarecirculatenometabmv2l58_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
                tbl_psarecirculatenometabmv2l60_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
                tbl_psarecirculatenometabmv2l62_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
                tbl_psarecirculatenometabmv2l64_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
                tbl_psarecirculatenometabmv2l66_0.apply();
            } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
                tbl_psarecirculatenometabmv2l68_0.apply();
            }
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psarecirculatenometabmv2l159() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psarecirculatenometabmv2l159 {
        actions = {
            psarecirculatenometabmv2l159();
        }
        const default_action = psarecirculatenometabmv2l159();
    }
    apply {
        tbl_psarecirculatenometabmv2l159.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psarecirculatenometabmv2l159_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
    @hidden table tbl_psarecirculatenometabmv2l159_0 {
        actions = {
            psarecirculatenometabmv2l159_0();
        }
        const default_action = psarecirculatenometabmv2l159_0();
    }
    apply {
        tbl_psarecirculatenometabmv2l159_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
