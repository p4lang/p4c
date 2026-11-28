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
    @name("P.tmp") int<32> tmp;
    @name("P.tmp_0") int<32> tmp_0;
    @name("P.tmp_1") int<32> tmp_1;
    @name("P.tmp_2") int<32> tmp_2;
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
    state parse_loop {
        tmp = index_0;
        packet.extract<H>(h.stack[tmp]);
        index_0 = index_0 + 32s1;
        transition select(h.stack[index_0 + -32s1].again) {
            8w0: parse_loop;
            default: accept;
        }
    }
    state parse_oob_first {
        tmp_0 = index_0;
        packet.extract<H>(h.stack[tmp_0]);
        index_0 = index_0 + 32s1;
        tmp_1 = index_0;
        packet.extract<H>(h.stack[tmp_1]);
        packet.extract<Tail>(h.tail);
        transition accept;
    }
    state parse_next_loop {
        packet.extract<H>(h.stack.next);
        transition select(h.stack.last.again) {
            8w0: parse_next_loop;
            default: accept;
        }
    }
    state parse_neg_index {
        index_0 = index_0 + -32s1;
        tmp_2 = index_0;
        packet.extract<H>(h.stack[tmp_2]);
        packet.extract<Tail>(h.tail);
        transition accept;
    }
}

control I(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
        h.result.setValid();
        h.result.error_code = 8w255;
        if (sm.parser_error == error.NoError) {
            h.result.error_code = 8w0;
        } else if (sm.parser_error == error.PacketTooShort) {
            h.result.error_code = 8w1;
        } else if (sm.parser_error == error.StackOutOfBounds) {
            h.result.error_code = 8w2;
        }
        if (h.stack[0].isValid()) {
            h.result.elem0_tag = h.stack[0].tag;
        } else {
            h.result.elem0_tag = 8w0;
        }
        if (h.tail.isValid()) {
            h.result.tail_valid = 8w1;
            h.result.tail_value = h.tail.value;
        } else {
            h.result.tail_valid = 8w0;
            h.result.tail_value = 8w0;
        }
        sm.egress_spec = 9w0;
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
