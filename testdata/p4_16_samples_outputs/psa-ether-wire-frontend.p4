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
    fwd_metadata_t fwd_metadata;
}

struct headers {
    ethernet_t ethernet;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    headers parsed_hdr_2;
    metadata user_meta_2;
    state start {
        parsed_hdr_2.ethernet.setInvalid();
        user_meta_2 = user_meta;
        transition CommonParser_start;
    }
    state CommonParser_start {
        buffer.extract<ethernet_t>(parsed_hdr_2.ethernet);
        transition start_0;
    }
    state start_0 {
        parsed_hdr = parsed_hdr_2;
        user_meta = user_meta_2;
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    headers parsed_hdr_3;
    metadata user_meta_3;
    state start {
        parsed_hdr_3.ethernet.setInvalid();
        user_meta_3 = user_meta;
        transition CommonParser_start_0;
    }
    state CommonParser_start_0 {
        buffer.extract<ethernet_t>(parsed_hdr_3.ethernet);
        transition start_1;
    }
    state start_1 {
        parsed_hdr = parsed_hdr_3;
        user_meta = user_meta_3;
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
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout headers hdr, in metadata meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

IngressPipeline<headers, metadata, empty_t, empty_t, empty_t, empty_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;

EgressPipeline<headers, metadata, empty_t, empty_t, empty_t, empty_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;

PSA_Switch<headers, metadata, headers, metadata, empty_t, empty_t, empty_t, empty_t, empty_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

