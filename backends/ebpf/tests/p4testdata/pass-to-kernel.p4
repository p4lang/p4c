#include <core.p4>
#include <psa.p4>
#include "common_headers.p4"

struct metadata {
}

struct headers {
    ethernet_t       ethernet;
}

parser IngressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_ingress_parser_input_metadata_t istd,
    in empty_t resubmit_meta,
    in empty_t recirculate_meta)
{
    state start {
        transition accept;
    }
}

control ingress(inout headers hdr,
                inout metadata user_meta,
                in  psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply {
        // setting drop=false and egress_port=0 should enforce sending packet up to the kernel stack
        ostd.drop = false;
        // ostd.egress_port left unspecified
    }
}

parser EgressParserImpl(
    packet_in buffer,
    out headers parsed_hdr,
    inout metadata user_meta,
    in psa_egress_parser_input_metadata_t istd,
    in metadata normal_meta,
    in empty_t clone_i2e_meta,
    in empty_t clone_e2e_meta)
{
    state start {
        transition accept;
    }
}

control egress(inout headers hdr,
               inout metadata user_meta,
               in  psa_egress_input_metadata_t  istd,
               inout psa_egress_output_metadata_t ostd)
{
    Counter<bit<32>, bit<32>>(1024, PSA_CounterType_t.PACKETS) eg_packets;

    apply {
        // this counter should not be incremented if packets are not coming from PSA ingress.
        eg_packets.count(0);
    }
}

control IngressDeparserImpl(
    packet_out packet,
    out empty_t clone_i2e_meta,
    out empty_t resubmit_meta,
    out metadata normal_meta,
    inout headers hdr,
    in metadata meta,
    in psa_ingress_output_metadata_t istd)
{
    apply {
    }
}

control EgressDeparserImpl(
    packet_out packet,
    out empty_t clone_e2e_meta,
    out empty_t recirculate_meta,
    inout headers hdr,
    in metadata meta,
    in psa_egress_output_metadata_t istd,
    in psa_egress_deparser_input_metadata_t edstd)
{
    apply {
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
               egress(),
               EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;


