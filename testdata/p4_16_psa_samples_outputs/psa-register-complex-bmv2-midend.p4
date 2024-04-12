#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
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
    @name("cIngress.tmp") bit<48> tmp;
    @noWarn("unused") @name(".send_to_port") action send_to_port_0() {
        ostd.drop = false;
        ostd.multicast_group = 32w0;
        ostd.egress_port = (bit<32>)hdr.ethernet.dstAddr;
    }
    @name("cIngress.regfile") Register<bit<48>, bit<32>>(32w128) regfile_0;
    @hidden action psaregistercomplexbmv2l60() {
        regfile_0.write(32w1, 48w3);
        regfile_0.write(32w2, 48w4);
        tmp = regfile_0.read(32w1);
        hdr.ethernet.dstAddr = regfile_0.read(32w1) + regfile_0.read(32w2) + 48w281474976710651;
    }
    @hidden table tbl_psaregistercomplexbmv2l60 {
        actions = {
            psaregistercomplexbmv2l60();
        }
        const default_action = psaregistercomplexbmv2l60();
    }
    @hidden table tbl_send_to_port {
        actions = {
            send_to_port_0();
        }
        const default_action = send_to_port_0();
    }
    apply {
        tbl_psaregistercomplexbmv2l60.apply();
        if (tmp + regfile_0.read(32w2) + 48w281474976710651 == 48w2) {
            tbl_send_to_port.apply();
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
    @hidden action psaregistercomplexbmv2l101() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaregistercomplexbmv2l101 {
        actions = {
            psaregistercomplexbmv2l101();
        }
        const default_action = psaregistercomplexbmv2l101();
    }
    apply {
        tbl_psaregistercomplexbmv2l101.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_metadata_t clone_e2e_meta, out empty_metadata_t recirculate_meta, inout headers_t hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psaregistercomplexbmv2l101_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psaregistercomplexbmv2l101_0 {
        actions = {
            psaregistercomplexbmv2l101_0();
        }
        const default_action = psaregistercomplexbmv2l101_0();
    }
    apply {
        tbl_psaregistercomplexbmv2l101_0.apply();
    }
}

IngressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(IngressParserImpl(), cIngress(), IngressDeparserImpl()) ip;
EgressPipeline<headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(EgressParserImpl(), cEgress(), EgressDeparserImpl()) ep;
PSA_Switch<headers_t, metadata_t, headers_t, metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t, empty_metadata_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
