#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
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
    bit<8> error_as_int_0;
    @hidden action act() {
        error_as_int_0 = 8w0;
    }
    @hidden action act_0() {
        error_as_int_0 = 8w1;
    }
    @hidden action act_1() {
        error_as_int_0 = 8w2;
    }
    @hidden action act_2() {
        error_as_int_0 = 8w3;
    }
    @hidden action act_3() {
        error_as_int_0 = 8w4;
    }
    @hidden action act_4() {
        error_as_int_0 = 8w5;
    }
    @hidden action act_5() {
        error_as_int_0 = 8w6;
    }
    @hidden action act_6() {
        error_as_int_0 = 8w7;
    }
    @hidden action act_7() {
        stdmeta.egress_spec = 9w1;
    }
    @hidden action act_8() {
        hdr.ethernet.dstAddr[7:0] = error_as_int_0;
    }
    @hidden table tbl_act {
        actions = {
            act_7();
        }
        const default_action = act_7();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_2 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    @hidden table tbl_act_3 {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    @hidden table tbl_act_4 {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    @hidden table tbl_act_5 {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    @hidden table tbl_act_6 {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    @hidden table tbl_act_7 {
        actions = {
            act_6();
        }
        const default_action = act_6();
    }
    @hidden table tbl_act_8 {
        actions = {
            act_8();
        }
        const default_action = act_8();
    }
    apply {
        tbl_act.apply();
        if (stdmeta.parser_error == error.NoError) {
            tbl_act_0.apply();
        } else if (stdmeta.parser_error == error.PacketTooShort) {
            tbl_act_1.apply();
        } else if (stdmeta.parser_error == error.NoMatch) {
            tbl_act_2.apply();
        } else if (stdmeta.parser_error == error.StackOutOfBounds) {
            tbl_act_3.apply();
        } else if (stdmeta.parser_error == error.HeaderTooShort) {
            tbl_act_4.apply();
        } else if (stdmeta.parser_error == error.ParserTimeout) {
            tbl_act_5.apply();
        } else if (stdmeta.parser_error == error.ParserInvalidArgument) {
            tbl_act_6.apply();
        } else {
            tbl_act_7.apply();
        }
        tbl_act_8.apply();
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

