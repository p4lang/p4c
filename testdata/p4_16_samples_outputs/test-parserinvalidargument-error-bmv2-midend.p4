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
    @hidden action testparserinvalidargumenterrorbmv2l59() {
        error_as_int_0 = 8w0;
    }
    @hidden action testparserinvalidargumenterrorbmv2l61() {
        error_as_int_0 = 8w1;
    }
    @hidden action testparserinvalidargumenterrorbmv2l63() {
        error_as_int_0 = 8w2;
    }
    @hidden action testparserinvalidargumenterrorbmv2l65() {
        error_as_int_0 = 8w3;
    }
    @hidden action testparserinvalidargumenterrorbmv2l67() {
        error_as_int_0 = 8w4;
    }
    @hidden action testparserinvalidargumenterrorbmv2l69() {
        error_as_int_0 = 8w5;
    }
    @hidden action testparserinvalidargumenterrorbmv2l71() {
        error_as_int_0 = 8w6;
    }
    @hidden action testparserinvalidargumenterrorbmv2l76() {
        error_as_int_0 = 8w7;
    }
    @hidden action testparserinvalidargumenterrorbmv2l57() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden action testparserinvalidargumenterrorbmv2l78() {
        hdr.ethernet.dstAddr[7:0] = error_as_int_0;
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l57 {
        actions = {
            testparserinvalidargumenterrorbmv2l57();
        }
        const default_action = testparserinvalidargumenterrorbmv2l57();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l59 {
        actions = {
            testparserinvalidargumenterrorbmv2l59();
        }
        const default_action = testparserinvalidargumenterrorbmv2l59();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l61 {
        actions = {
            testparserinvalidargumenterrorbmv2l61();
        }
        const default_action = testparserinvalidargumenterrorbmv2l61();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l63 {
        actions = {
            testparserinvalidargumenterrorbmv2l63();
        }
        const default_action = testparserinvalidargumenterrorbmv2l63();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l65 {
        actions = {
            testparserinvalidargumenterrorbmv2l65();
        }
        const default_action = testparserinvalidargumenterrorbmv2l65();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l67 {
        actions = {
            testparserinvalidargumenterrorbmv2l67();
        }
        const default_action = testparserinvalidargumenterrorbmv2l67();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l69 {
        actions = {
            testparserinvalidargumenterrorbmv2l69();
        }
        const default_action = testparserinvalidargumenterrorbmv2l69();
    }
    @hidden table tbl_testparserinvalidargumenterrorbmv2l71 {
        actions = {
            testparserinvalidargumenterrorbmv2l71();
        }
        const default_action = testparserinvalidargumenterrorbmv2l71();
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
    apply {
        tbl_testparserinvalidargumenterrorbmv2l57.apply();
        if (stdmeta.parser_error == error.NoError) {
            tbl_testparserinvalidargumenterrorbmv2l59.apply();
        } else if (stdmeta.parser_error == error.PacketTooShort) {
            tbl_testparserinvalidargumenterrorbmv2l61.apply();
        } else if (stdmeta.parser_error == error.NoMatch) {
            tbl_testparserinvalidargumenterrorbmv2l63.apply();
        } else if (stdmeta.parser_error == error.StackOutOfBounds) {
            tbl_testparserinvalidargumenterrorbmv2l65.apply();
        } else if (stdmeta.parser_error == error.HeaderTooShort) {
            tbl_testparserinvalidargumenterrorbmv2l67.apply();
        } else if (stdmeta.parser_error == error.ParserTimeout) {
            tbl_testparserinvalidargumenterrorbmv2l69.apply();
        } else if (stdmeta.parser_error == error.ParserInvalidArgument) {
            tbl_testparserinvalidargumenterrorbmv2l71.apply();
        } else {
            tbl_testparserinvalidargumenterrorbmv2l76.apply();
        }
        tbl_testparserinvalidargumenterrorbmv2l78.apply();
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
