#include <core.p4>
#include <psa.p4>

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
    @noWarnUnused @name(".send_to_port") action send_to_port() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (PortIdUint_t)hdr.ethernet.dstAddr;
    }
    @noWarnUnused @name(".ingress_drop") action ingress_drop() {
        ostd.drop = true;
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port();
        }
        const default_action = send_to_port();
    }
    @hidden table tbl_ingress_drop {
        actions = {
            ingress_drop();
        }
        const default_action = ingress_drop();
    }
    apply {
        tbl_send_to_port.apply();
        if (hdr.ethernet.dstAddr == 48w0) {
            tbl_ingress_drop.apply();
        }
    }
}

parser EgressParserImpl(packet_in buffer, out headers_t hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_metadata_t normal_meta, in empty_metadata_t clone_i2e_meta, in empty_metadata_t clone_e2e_meta) {
    state start {
        transition accept;
    }
}

control cEgress(inout headers_t hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_metadata_t clone_i2e_meta, out empty_metadata_t resubmit_meta, out empty_metadata_t normal_meta, inout headers_t hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psaunicastordropbmv2l99() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaunicastordropbmv2l99 {
        actions = {
            psaunicastordropbmv2l99();
        }
        const default_action = psaunicastordropbmv2l99();
    }
    apply {
        tbl_psaunicastordropbmv2l99.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaunicastordropbmv2l99_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaunicastordropbmv2l99_0 {
        actions = {
            psaunicastordropbmv2l99_0();
        }
        const default_action = psaunicastordropbmv2l99_0();
    }
    apply {
        tbl_psaunicastordropbmv2l99_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;

EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;

PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

