#include <core.p4>
#include <psa.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct fwd_metadata_t {
}

struct empty_t {
}

struct metadata {
}

struct headers {
    ethernet_t ethernet;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    ethernet_t parsed_hdr_0_ethernet;
    state start {
        parsed_hdr_0_ethernet.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr_0_ethernet);
        parsed_hdr.ethernet = parsed_hdr_0_ethernet;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    ethernet_t parsed_hdr_1_ethernet;
    state start {
        parsed_hdr_1_ethernet.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr_1_ethernet);
        parsed_hdr.ethernet = parsed_hdr_1_ethernet;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    apply {
    }
}

control egress(inout headers hdr, inout metadata user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers hdr, in metadata meta, in psa_ingress_output_metadata_t istd) {
    @hidden action act() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action act_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act_0.apply();
    }
}

IngressPipeline<headers, metadata, empty_t, empty_t, empty_t, empty_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_t, empty_t, empty_t, empty_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_t, empty_t, empty_t, empty_t, empty_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

