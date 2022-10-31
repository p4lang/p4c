#include <core.p4>
#include <bmv2/psa.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    @name("cIngress.meta") psa_ingress_output_metadata_t meta_0;
    @name("cIngress.egress_port") PortId_t egress_port_0;
    @name("cIngress.meta") psa_ingress_output_metadata_t meta_3;
    @name("cIngress.egress_port") PortId_t egress_port_3;
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)(PortIdUint_t)hdr.ethernet.dstAddr;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        meta_3 = ostd;
        egress_port_3 = (PortId_t)32w0xfffffffa;
        meta_3.drop = false;
        meta_3.multicast_group = (MulticastGroup_t)32w0;
        meta_3.egress_port = egress_port_3;
        ostd = meta_3;
    }
    @name("cIngress.record_ingress_ports_in_pkt") action record_ingress_ports_in_pkt() {
        hdr.output_data.word1 = (PortIdUint_t)istd.ingress_port;
    }
    apply {
        if (hdr.ethernet.dstAddr[3:0] >= 4w4) {
            record_ingress_ports_in_pkt();
            send_to_port_1();
        } else {
            send_to_port_2();
        }
        int_packet_path_0 = 32w8;
        if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
            int_packet_path_0 = 32w1;
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            int_packet_path_0 = 32w2;
        } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            int_packet_path_0 = 32w3;
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
            int_packet_path_0 = 32w4;
        } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            int_packet_path_0 = 32w5;
        } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            int_packet_path_0 = 32w6;
        } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            int_packet_path_0 = 32w7;
        }
        if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
            hdr.output_data.word2 = int_packet_path_0;
        } else {
            hdr.output_data.word0 = int_packet_path_0;
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
    apply {
        e_0.apply();
        if (istd.egress_port == (PortId_t)32w0xfffffffa) {
            hdr.output_data.word3 = 32w8;
            if (istd.packet_path == PSA_PacketPath_t.NORMAL) {
                hdr.output_data.word3 = 32w1;
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
                hdr.output_data.word3 = 32w2;
            } else if (istd.packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
                hdr.output_data.word3 = 32w3;
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_I2E) {
                hdr.output_data.word3 = 32w4;
            } else if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
                hdr.output_data.word3 = 32w5;
            } else if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
                hdr.output_data.word3 = 32w6;
            } else if (istd.packet_path == PSA_PacketPath_t.RECIRCULATE) {
                hdr.output_data.word3 = 32w7;
            }
        }
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
        buffer.emit<output_data_t>(hdr.output_data);
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
