#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

@command_line("--loopsUnroll") header Ctrl {
    bit<8> mode;
}

header H {
    bit<8> again;
    bit<8> tag;
}

header Tail {
    bit<8> value;
}

header Result {
    bit<8> error_code;
    bit<8> elem0_tag;
    bit<8> tail_valid;
    bit<8> tail_value;
}

struct Headers {
    Ctrl   ctrl;
    H[1]   stack;
    Tail   tail;
    Result result;
}

struct Meta {
}

parser P(packet_in packet, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("P.index") int<32> index_0;
    H discard;
    H discard_0;
    H discard_1;
    state stateOutOfBound {
        verify(false, error.StackOutOfBounds);
        transition reject;
    }
    state parse_loop {
        packet.extract<H>(h.stack[32s0]);
        index_0 = index_0 + 32s1;
        transition select(h.stack[32s0].again) {
            8w0: parse_loop1;
            default: accept;
        }
    }
    state parse_loop1 {
        packet.extract<H>(discard_1);
        index_0 = index_0 + 32s1;
        transition stateOutOfBound;
    }
    state parse_neg_index {
        index_0 = index_0 + -32s1;
        packet.extract<H>(discard);
        packet.extract<Tail>(h.tail);
        transition accept;
    }
    state parse_next_loop {
        packet.extract<H>(h.stack[32w0]);
        transition select(h.stack[32w0].again) {
            8w0: parse_next_loop1;
            default: accept;
        }
    }
    state parse_next_loop1 {
        transition stateOutOfBound;
    }
    state parse_oob_first {
        packet.extract<H>(h.stack[32s0]);
        index_0 = index_0 + 32s1;
        packet.extract<H>(discard_0);
        packet.extract<Tail>(h.tail);
        transition accept;
    }
    state start {
        packet.extract<Ctrl>(h.ctrl);
        index_0 = 32s0;
        transition select(h.ctrl.mode) {
            8w0: parse_loop;
            8w1: parse_oob_first;
            8w2: parse_next_loop;
            8w3: parse_neg_index;
            default: accept;
        }
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @hidden action issue5861bmv2l105() {
        h.result.error_code = 8w0;
    }
    @hidden action issue5861bmv2l107() {
        h.result.error_code = 8w1;
    }
    @hidden action issue5861bmv2l109() {
        h.result.error_code = 8w2;
    }
    @hidden action issue5861bmv2l102() {
        h.result.setValid();
        h.result.error_code = 8w255;
    }
    @hidden action issue5861bmv2l112() {
        h.result.elem0_tag = h.stack[0].tag;
    }
    @hidden action issue5861bmv2l114() {
        h.result.elem0_tag = 8w0;
    }
    @hidden action issue5861bmv2l117() {
        h.result.tail_valid = 8w1;
        h.result.tail_value = h.tail.value;
    }
    @hidden action issue5861bmv2l120() {
        h.result.tail_valid = 8w0;
        h.result.tail_value = 8w0;
    }
    @hidden action issue5861bmv2l123() {
        sm.egress_spec = 9w0;
    }
    @hidden table tbl_issue5861bmv2l102 {
        actions = {
            issue5861bmv2l102();
        }
        const default_action = issue5861bmv2l102();
    }
    @hidden table tbl_issue5861bmv2l105 {
        actions = {
            issue5861bmv2l105();
        }
        const default_action = issue5861bmv2l105();
    }
    @hidden table tbl_issue5861bmv2l107 {
        actions = {
            issue5861bmv2l107();
        }
        const default_action = issue5861bmv2l107();
    }
    @hidden table tbl_issue5861bmv2l109 {
        actions = {
            issue5861bmv2l109();
        }
        const default_action = issue5861bmv2l109();
    }
    @hidden table tbl_issue5861bmv2l112 {
        actions = {
            issue5861bmv2l112();
        }
        const default_action = issue5861bmv2l112();
    }
    @hidden table tbl_issue5861bmv2l114 {
        actions = {
            issue5861bmv2l114();
        }
        const default_action = issue5861bmv2l114();
    }
    @hidden table tbl_issue5861bmv2l117 {
        actions = {
            issue5861bmv2l117();
        }
        const default_action = issue5861bmv2l117();
    }
    @hidden table tbl_issue5861bmv2l120 {
        actions = {
            issue5861bmv2l120();
        }
        const default_action = issue5861bmv2l120();
    }
    @hidden table tbl_issue5861bmv2l123 {
        actions = {
            issue5861bmv2l123();
        }
        const default_action = issue5861bmv2l123();
    }
    apply {
        tbl_issue5861bmv2l102.apply();
        if (sm.parser_error == error.NoError) {
            tbl_issue5861bmv2l105.apply();
        } else if (sm.parser_error == error.PacketTooShort) {
            tbl_issue5861bmv2l107.apply();
        } else if (sm.parser_error == error.StackOutOfBounds) {
            tbl_issue5861bmv2l109.apply();
        }
        if (h.stack[0].isValid()) {
            tbl_issue5861bmv2l112.apply();
        } else {
            tbl_issue5861bmv2l114.apply();
        }
        if (h.tail.isValid()) {
            tbl_issue5861bmv2l117.apply();
        } else {
            tbl_issue5861bmv2l120.apply();
        }
        tbl_issue5861bmv2l123.apply();
    }
}

control E(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control Verify(inout Headers h, inout Meta m) {
    apply {
    }
}

control Update(inout Headers h, inout Meta m) {
    apply {
    }
}

control D(packet_out packet, in Headers h) {
    apply {
        packet.emit<Result>(h.result);
    }
}

V1Switch<Headers, Meta>(P(), Verify(), I(), E(), Update(), D()) main;
