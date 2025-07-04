#include <core.p4>
#include <bmv2/psa.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct fwd_metadata_t {
}

struct empty_t {
}

struct metadata_t {
    bit<8> field;
}

struct headers {
    ethernet_t ethernet;
}

parser IngressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata_t user_meta, in psa_ingress_parser_input_metadata_t istd, in empty_t resubmit_meta, in empty_t recirculate_meta) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition accept;
    }
}

parser EgressParserImpl(packet_in buffer, out headers parsed_hdr, inout metadata_t user_meta, in psa_egress_parser_input_metadata_t istd, in empty_t normal_meta, in empty_t clone_i2e_meta, in empty_t clone_e2e_meta) {
    state start {
        parsed_hdr.ethernet.setInvalid();
        buffer.extract<ethernet_t>(parsed_hdr.ethernet);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata_t user_meta, in psa_ingress_input_metadata_t istd, inout psa_ingress_output_metadata_t ostd) {
    @corelib @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("ingress.set_output") action set_output(@name("port") bit<32> port) {
        ostd.egress_port = port;
    }
    @name("ingress.match_meta") table match_meta_0 {
        key = {
            hdr.ethernet.dstAddr[7:0]: exact @name("user_meta.field");
        }
        actions = {
            set_output();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action psametadatabmv2l104() {
        user_meta.field = hdr.ethernet.dstAddr[7:0];
    }
    @hidden table tbl_psametadatabmv2l104 {
        actions = {
            psametadatabmv2l104();
        }
        const default_action = psametadatabmv2l104();
    }
    apply {
        tbl_psametadatabmv2l104.apply();
        match_meta_0.apply();
    }
}

control egress(inout headers hdr, inout metadata_t user_meta, in psa_egress_input_metadata_t istd, inout psa_egress_output_metadata_t ostd) {
    apply {
    }
}

control IngressDeparserImpl(packet_out buffer, out empty_t clone_i2e_meta, out empty_t resubmit_meta, out empty_t normal_meta, inout headers hdr, in metadata_t meta, in psa_ingress_output_metadata_t istd) {
    @hidden action psametadatabmv2l121() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psametadatabmv2l121 {
        actions = {
            psametadatabmv2l121();
        }
        const default_action = psametadatabmv2l121();
    }
    apply {
        tbl_psametadatabmv2l121.apply();
    }
}

control EgressDeparserImpl(packet_out buffer, out empty_t clone_e2e_meta, out empty_t recirculate_meta, inout headers hdr, in metadata_t meta, in psa_egress_output_metadata_t istd, in psa_egress_deparser_input_metadata_t edstd) {
    @hidden action psametadatabmv2l121_0() {
        buffer.emit<ethernet_t>(hdr.ethernet);
    }
    @hidden table tbl_psametadatabmv2l121_0 {
        actions = {
            psametadatabmv2l121_0();
        }
        const default_action = psametadatabmv2l121_0();
    }
    apply {
        tbl_psametadatabmv2l121_0.apply();
    }
}

IngressPipeline<headers, metadata_t, empty_t, empty_t, empty_t, empty_t>(IngressParserImpl(), ingress(), IngressDeparserImpl()) ip;
EgressPipeline<headers, metadata_t, empty_t, empty_t, empty_t, empty_t>(EgressParserImpl(), egress(), EgressDeparserImpl()) ep;
PSA_Switch<headers, metadata_t, headers, metadata_t, empty_t, empty_t, empty_t, empty_t, empty_t>(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
