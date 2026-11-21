/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>
@command_line("--loopsUnroll")

header Ctrl {
    bit<8> mode;
}

// 16-bit element header so out-of-range extraction must advance by 16 bits.
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

struct Meta {}

parser P(packet_in packet, out Headers h, inout Meta m,
         inout standard_metadata_t sm) {
    int<32> index;

    state start {
        packet.extract(h.ctrl);
        index = 0;
        transition select(h.ctrl.mode) {
            8w0: parse_loop;
            8w1: parse_oob_first;
            8w2: parse_next_loop;
            8w3: parse_neg_index;
            default: accept;
        }
    }

    // Mode 0: reporter's explicit-index loop.
    state parse_loop {
        packet.extract(h.stack[index]);
        index += 1;
        transition select(h.stack[index - 1].again) {
            8w0: parse_loop;
            default: accept;
        }
    }

    // Mode 1: explicit in-range extract, then out-of-range explicit extract
    // (index == 1 for H[1]), then a defined trailing extract.
    state parse_oob_first {
        packet.extract(h.stack[index]);
        index += 1;
        transition parse_oob_second;
    }

    state parse_oob_second {
        packet.extract(h.stack[index]);
        index += 1;
        packet.extract(h.tail);
        transition accept;
    }

    // Mode 2: .next exhaustion control (must still report StackOutOfBounds).
    state parse_next_loop {
        packet.extract(h.stack.next);
        transition select(h.stack.last.again) {
            8w0: parse_next_loop;
            default: accept;
        }
    }

    // Mode 3: negative runtime index (-1) followed by a defined trailing extract.
    state parse_neg_index {
        index = index - 32s1;
        packet.extract(h.stack[index]);
        packet.extract(h.tail);
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
    apply {}
}

control Verify(inout Headers h, inout Meta m) {
    apply {}
}

control Update(inout Headers h, inout Meta m) {
    apply {}
}

control D(packet_out packet, in Headers h) {
    apply {
        packet.emit(h.result);
    }
}

V1Switch(P(), Verify(), I(), E(), Update(), D()) main;
