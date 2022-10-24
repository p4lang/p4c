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

control packet_path_to_int(in PSA_PacketPath_t packet_path, out bit<32> ret) {
    apply {
        ret = 8;
        if (packet_path == PSA_PacketPath_t.NORMAL) {
            ret = 1;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            ret = 2;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            ret = 3;
        } else if (packet_path == PSA_PacketPath_t.CLONE_I2E) {
            ret = 4;
        } else if (packet_path == PSA_PacketPath_t.CLONE_E2E) {
            ret = 5;
        } else if (packet_path == PSA_PacketPath_t.RESUBMIT) {
            ret = 6;
        } else if (packet_path == PSA_PacketPath_t.RECIRCULATE) {
            ret = 7;
        }
    }
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    apply {
        multicast(ostd, (MulticastGroup_t)(MulticastGroupUint_t)hdr.ethernet.dstAddr);
        ostd.class_of_service = (ClassOfService_t)(ClassOfServiceUint_t)hdr.ethernet.srcAddr[0:0];
    }
}

parser EgressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        pkt.extract(hdr.ethernet);
        pkt.extract(hdr.output_data);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
        hdr.output_data.word0 = (bit<32>)istd.egress_port;
        hdr.output_data.word1 = (bit<32>)(EgressInstanceUint_t)istd.instance;
        packet_path_to_int.apply(istd.packet_path, hdr.output_data.word2);
        hdr.output_data.word3 = (bit<32>)(ClassOfServiceUint_t)istd.class_of_service;
    }
}

control CommonDeparserImpl(packet_out packet, inout headers_t hdr) {
    apply {
        packet.emit(hdr.ethernet);
        packet.emit(hdr.output_data);
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    CommonDeparserImpl() cp;
    apply {
        cp.apply(buffer, hdr);
    }
}

IngressPipeline(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
