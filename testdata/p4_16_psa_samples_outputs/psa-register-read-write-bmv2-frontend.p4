#include <core.p4>
#include <bmv2/psa.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct empty_metadata_t {
}

struct metadata_t {
}

struct headers_t {
    ethernet_t ethernet;
}

parser IngressParserImpl(packet_in pkt, out headers_t hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_metadata_t resubmit_meta, in empty_metadata_t recirculate_meta) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout headers_t hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @name("cIngress.meta") psa_ingress_output_metadata_t meta_0;
    @name("cIngress.egress_port") PortId_t egress_port_0;
    @name("cIngress.meta") psa_ingress_output_metadata_t meta_1;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)(PortIdUint_t)hdr.ethernet.dstAddr;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @noWarn("unused") @name(".ingress_drop") action ingress_drop_0() {
        meta_1 = ostd;
        meta_1.drop = true;
        ostd = meta_1;
    }
    @name("cIngress.regfile") Register<EthernetAddress, bit<32>>(32w128) regfile_0;
    apply {
        regfile_0.write(32w1, 48w3);
        hdr.ethernet.dstAddr = regfile_0.read(32w1);
        send_to_port_0();
        if (hdr.ethernet.dstAddr == 48w0) {
            ingress_drop_0();
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        buffer.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    apply {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
