#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header custom_var_len_t {
    varbit<16> options;
}

struct headers_t {
    ethernet_t       ethernet;
    custom_var_len_t custom_var_len;
}

struct metadata_t {
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        packet.extract<custom_var_len_t>(hdr.custom_var_len, (bit<32>)hdr.ethernet.etherType[7:0]);
        transition accept;
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.error_as_int") bit<8> error_as_int_0;
    @hidden action testparserinvalidargumenterrorbmv2l68() {
        error_as_int_0 = 8w0;
    }
    @hidden action testparserinvalidargumenterrorbmv2l70() {
        error_as_int_0 = 8w1;
    }
    @hidden action testparserinvalidargumenterrorbmv2l72() {
        error_as_int_0 = 8w2;
    }
    @hidden action testparserinvalidargumenterrorbmv2l74() {
        error_as_int_0 = 8w3;
    }
    @hidden action testparserinvalidargumenterrorbmv2l76() {
        error_as_int_0 = 8w4;
    }
    @hidden action testparserinvalidargumenterrorbmv2l78() {
        error_as_int_0 = 8w5;
    }
    @hidden action testparserinvalidargumenterrorbmv2l80() {
        error_as_int_0 = 8w6;
    }
    @hidden action testparserinvalidargumenterrorbmv2l85() {
        error_as_int_0 = 8w7;
    }
    @hidden action testparserinvalidargumenterrorbmv2l66() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden action testparserinvalidargumenterrorbmv2l87() {
        hdr.ethernet.dstAddr[7:0] = error_as_int_0;
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l66 {
        actions = {
            testparserinvalidargumenterrorbmv2l66();
        }
        const default_action = testparserinvalidargumenterrorbmv2l66();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l68 {
        actions = {
            testparserinvalidargumenterrorbmv2l68();
        }
        const default_action = testparserinvalidargumenterrorbmv2l68();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l70 {
        actions = {
            testparserinvalidargumenterrorbmv2l70();
        }
        const default_action = testparserinvalidargumenterrorbmv2l70();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l72 {
        actions = {
            testparserinvalidargumenterrorbmv2l72();
        }
        const default_action = testparserinvalidargumenterrorbmv2l72();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l74 {
        actions = {
            testparserinvalidargumenterrorbmv2l74();
        }
        const default_action = testparserinvalidargumenterrorbmv2l74();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l76 {
        actions = {
            testparserinvalidargumenterrorbmv2l76();
        }
        const default_action = testparserinvalidargumenterrorbmv2l76();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l78 {
        actions = {
            testparserinvalidargumenterrorbmv2l78();
        }
        const default_action = testparserinvalidargumenterrorbmv2l78();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l80 {
        actions = {
            testparserinvalidargumenterrorbmv2l80();
        }
        const default_action = testparserinvalidargumenterrorbmv2l80();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l85 {
        actions = {
            testparserinvalidargumenterrorbmv2l85();
        }
        const default_action = testparserinvalidargumenterrorbmv2l85();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l87 {
        actions = {
            testparserinvalidargumenterrorbmv2l87();
        }
        const default_action = testparserinvalidargumenterrorbmv2l87();
    }
    apply {
        tbl_testparserinvalidargumenterrorbmv2l66.apply();
        if (stdmeta.parser_error == error.NoError) {
            tbl_testparserinvalidargumenterrorbmv2l68.apply();
        } else if (stdmeta.parser_error == error.PacketTooShort) {
            tbl_testparserinvalidargumenterrorbmv2l70.apply();
        } else if (stdmeta.parser_error == error.NoMatch) {
            tbl_testparserinvalidargumenterrorbmv2l72.apply();
        } else if (stdmeta.parser_error == error.StackOutOfBounds) {
            tbl_testparserinvalidargumenterrorbmv2l74.apply();
        } else if (stdmeta.parser_error == error.HeaderTooShort) {
            tbl_testparserinvalidargumenterrorbmv2l76.apply();
        } else if (stdmeta.parser_error == error.ParserTimeout) {
            tbl_testparserinvalidargumenterrorbmv2l78.apply();
        } else if (stdmeta.parser_error == error.ParserInvalidArgument) {
            tbl_testparserinvalidargumenterrorbmv2l80.apply();
        } else {
            tbl_testparserinvalidargumenterrorbmv2l85.apply();
        }
        tbl_testparserinvalidargumenterrorbmv2l87.apply();
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;
