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
    @name("cIngress.egress_port") PortId_t egress_port_3;
    @noWarn("unused") @name(".send_to_port") action send_to_port_1() {
        meta_0 = ostd;
        egress_port_0 = (PortId_t)32w0xfffffffa;
        meta_0.drop = false;
        meta_0.multicast_group = (MulticastGroup_t)32w0;
        meta_0.egress_port = egress_port_0;
        ostd = meta_0;
    }
    @noWarn("unused") @name(".send_to_port") action send_to_port_2() {
        meta_1 = ostd;
        egress_port_3 = (PortId_t)(PortIdUint_t)hdr.ethernet.dstAddr;
        meta_1.drop = false;
        meta_1.multicast_group = (MulticastGroup_t)32w0;
        meta_1.egress_port = egress_port_3;
        ostd = meta_1;
    }
    apply {
        if (hdr.ethernet.dstAddr == 48w8 && istd.packet_path != PSA_PacketPath_t.RECIRCULATE) {
            send_to_port_1();
        } else {
            send_to_port_2();
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
    @name("cEgress.meta") psa_egress_output_metadata_t meta_5;
    @noWarn("unused") @name(".egress_drop") action egress_drop_0() {
        meta_5 = ostd;
        meta_5.drop = true;
        ostd = meta_5;
    }
    @name("cEgress.clone") action clone_1() {
        ostd.clone = true;
        ostd.clone_session_id = (CloneSessionId_t)16w8;
    }
    apply {
        if (istd.packet_path == PSA_PacketPath_t.CLONE_E2E) {
            hdr.ethernet.etherType = 16w0xface;
        } else {
            clone_1();
            if (hdr.ethernet.dstAddr == 48w9) {
                egress_drop_0();
                ostd.clone_session_id = (CloneSessionId_t)16w9;
            }
            if (istd.egress_port == (PortId_t)32w0xfffffffa) {
                hdr.ethernet.srcAddr = 48w0xbeef;
                ostd.clone_session_id = (CloneSessionId_t)16w10;
            } else {
                if (hdr.ethernet.dstAddr == 48w8) {
                    ostd.clone_session_id = (CloneSessionId_t)16w11;
                }
                hdr.ethernet.srcAddr = 48w0xcafe;
            }
        }
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
