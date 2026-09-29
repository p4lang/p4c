#include <core.p4>

#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct headers_t {
    ethernet_t eth;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("parserImpl.i") bit<8> i;
    @name("parserImpl.out1") bit<8> out1;
    @name("parserImpl.out2") bit<8> out2;
    @name("parserImpl.out3") bit<8> out3_0;
    @name("parserImpl.out4") bit<8> out4_0;
    @name("parserImpl.fooparser.tmp") bit<8> fooparser_tmp;
    @name("parserImpl.fooparser.i") bit<8> fooparser_i;
    @name("parserImpl.fooparser.i") bit<8> fooparser_i_0;
    state start {
        packet.extract<ethernet_t>(hdr.eth);
        i = hdr.eth.srcAddr[15:8];
        transition fooparser_start;
    }
    state fooparser_start {
        fooparser_tmp = i;
        fooparser_i = i + 8w1;
        out1 = fooparser_tmp;
        out2 = fooparser_i;
        fooparser_i_0 = fooparser_i + 8w2;
        out3_0 = fooparser_i_0;
        out4_0 = fooparser_i;
        transition start_0;
    }
    state start_0 {
        log_msg<tuple<bit<8>, bit<8>, bit<8>, bit<8>, bit<8>>>("i={} out1={} out2={} out3={} out4={}", { i, out1, out2, out3_0, out4_0 });
        transition accept;
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.i") bit<8> i_1;
    @name("ingressImpl.out1") bit<8> out1_1;
    @name("ingressImpl.out2") bit<8> out2_1;
    @name("ingressImpl.out3") bit<8> out3_1;
    @name("ingressImpl.out4") bit<8> out4_1;
    @name("ingressImpl.fooctrl.tmp") bit<8> fooctrl_tmp;
    @name("ingressImpl.fooctrl.i") bit<8> fooctrl_i;
    @name("ingressImpl.fooctrl.i") bit<8> fooctrl_i_0;
    @name("ingressImpl.i_0") bit<8> i_4;
    @name("ingressImpl.out1_0") bit<8> out1_4;
    @name("ingressImpl.out2_0") bit<8> out2_4;
    @name("ingressImpl.retval") bit<8> retval;
    @name("ingressImpl.tmp") bit<8> tmp_0;
    @name("ingressImpl.i") bit<8> i_7;
    @name("ingressImpl.i") bit<8> i_8;
    @name("ingressImpl.inlinedRetval") bit<8> inlinedRetval_0;
    apply {
        i_1 = hdr.eth.srcAddr[7:0];
        fooctrl_tmp = i_1;
        fooctrl_i = i_1 + 8w1;
        out1_1 = fooctrl_tmp;
        out2_1 = fooctrl_i;
        fooctrl_i_0 = fooctrl_i + 8w2;
        out3_1 = fooctrl_i_0;
        out4_1 = fooctrl_i;
        log_msg<tuple<bit<8>, bit<8>, bit<8>, bit<8>, bit<8>>>("i={} out1={} out2={} out3={} out4={}", { i_1, out1_1, out2_1, out3_1, out4_1 });
        i_4 = i_1;
        tmp_0 = i_4;
        i_7 = i_4 + 8w1;
        out1_4 = tmp_0;
        i_8 = i_7 + 8w2;
        out2_4 = i_8;
        retval = i_7;
        out1_1 = out1_4;
        out2_1 = out2_4;
        inlinedRetval_0 = retval;
        out3_1 = inlinedRetval_0;
        log_msg<tuple<bit<8>, bit<8>, bit<8>, bit<8>>>("i={} out1={} out2={} out3={}", { i_1, out1_1, out2_1, out3_1 });
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
