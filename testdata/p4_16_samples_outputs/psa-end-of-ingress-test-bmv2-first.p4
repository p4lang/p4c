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
        ret = 32w8;
        if (packet_path == PSA_PacketPath_t.NORMAL) {
            ret = 32w1;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_UNICAST) {
            ret = 32w2;
        } else if (packet_path == PSA_PacketPath_t.NORMAL_MULTICAST) {
            ret = 32w3;
        } else if (packet_path == PSA_PacketPath_t.CLONE_I2E) {
            ret = 32w4;
        } else if (packet_path == PSA_PacketPath_t.CLONE_E2E) {
            ret = 32w5;
        } else if (packet_path == PSA_PacketPath_t.RESUBMIT) {
            ret = 32w6;
        } else if (packet_path == PSA_PacketPath_t.RECIRCULATE) {
            ret = 32w7;
        }
    }
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        pkt.extract<output_data_t>(hdr.output_data);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("packet_path_to_int") packet_path_to_int() packet_path_to_int_inst;
    apply {
        ostd.drop = (bool)hdr.ethernet.dstAddr[40:40];
        if (istd.packet_path == PSA_PacketPath_t.RESUBMIT) {
            ;
        } else {
            ostd.resubmit = (bool)hdr.ethernet.dstAddr[32:32];
        }
        ostd.multicast_group = (MulticastGroup_t)(MulticastGroupUint_t)hdr.ethernet.dstAddr[31:16];
        ostd.egress_port = (PortId_t)(PortIdUint_t)hdr.ethernet.dstAddr[15:0];
        packet_path_to_int_inst.apply(istd.packet_path, hdr.output_data.word0);
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
    Register<bit<16>, bit<8>>(32w256) egress_pkt_seen;
    @name("packet_path_to_int") packet_path_to_int() packet_path_to_int_inst_0;
    apply {
        bit<8> idx = hdr.ethernet.etherType[7:0];
        bit<16> cur_count = egress_pkt_seen.read(idx);
        if (hdr.ethernet.etherType[15:8] == 8w0xc0) {
            hdr.output_data.word0 = (bit<32>)cur_count;
        } else if (hdr.ethernet.etherType[15:8] == 8w0xc1) {
            bit<16> write_data = hdr.ethernet.srcAddr[15:0];
            egress_pkt_seen.write(idx, write_data);
        } else {
            if (hdr.ethernet.etherType < 16w256) {
                egress_pkt_seen.write(idx, 16w1);
            }
            hdr.output_data.word1 = (bit<32>)istd.egress_port;
            hdr.output_data.word2 = (bit<32>)(EgressInstanceUint_t)istd.instance;
            packet_path_to_int_inst_0.apply(istd.packet_path, hdr.output_data.word3);
        }
    }
}

control CommonDeparserImpl(packet_out packet, inout headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<output_data_t>(hdr.output_data);
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

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
