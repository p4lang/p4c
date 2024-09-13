#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct metadata {
    bit<16> tmp_port;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers {
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ParserImpl.tmp_port") bit<16> tmp_port_0;
    @name("ParserImpl.x_1") bit<16> x;
    @name("ParserImpl.retval") bit<16> retval;
    @name("ParserImpl.x_2") bit<16> x_6;
    @name("ParserImpl.retval") bit<16> retval_0;
    state start {
        x = (bit<16>)standard_metadata.ingress_port;
        retval = x + 16w1;
        tmp_port_0 = retval;
        transition start_0;
    }
    state start_0 {
        packet.extract<ethernet_t>(hdr.ethernet);
        x_6 = hdr.ethernet.etherType;
        retval_0 = x_6 + 16w1;
        hdr.ethernet.etherType = retval_0;
        meta.tmp_port = tmp_port_0;
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.smeta") standard_metadata_t smeta_0;
    @name("ingress.x_3") bit<16> x_7;
    @name("ingress.retval_0") bit<16> retval_3;
    @name("ingress.tmp") bit<16> tmp;
    @name("ingress.tmp_0") bit<16> tmp_0;
    @name("ingress.tmp_1") bit<16> tmp_1;
    @name("ingress.x_0") bit<16> x_8;
    @name("ingress.retval") bit<16> retval_4;
    @name("ingress.x_4") bit<16> x_9;
    @name("ingress.retval") bit<16> retval_5;
    @name("ingress.x_5") bit<16> x_10;
    @name("ingress.retval") bit<16> retval_6;
    @name(".my_drop") action my_drop_0() {
        smeta_0 = standard_metadata;
        mark_to_drop(smeta_0);
        standard_metadata = smeta_0;
    }
    @name("ingress.set_port") action set_port(@name("output_port") bit<9> output_port) {
        standard_metadata.egress_spec = output_port;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            hdr.ethernet.dstAddr: exact @name("hdr.ethernet.dstAddr");
        }
        actions = {
            set_port();
            my_drop_0();
        }
        default_action = my_drop_0();
    }
    apply {
        mac_da_0.apply();
        x_7 = hdr.ethernet.srcAddr[15:0];
        tmp = x_7;
        x_8 = x_7;
        retval_4 = x_8 + 16w1;
        tmp_0 = retval_4;
        tmp_1 = tmp + tmp_0;
        retval_3 = tmp_1;
        hdr.ethernet.srcAddr[15:0] = retval_3;
        x_9 = hdr.ethernet.srcAddr[15:0];
        retval_5 = x_9 + 16w1;
        hdr.ethernet.srcAddr[15:0] = retval_5;
        x_10 = hdr.ethernet.etherType;
        retval_6 = x_10 + 16w1;
        hdr.ethernet.etherType = retval_6;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;
