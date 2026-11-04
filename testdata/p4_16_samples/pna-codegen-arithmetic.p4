/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

#ifndef ARITH_WIDTH
#define ARITH_WIDTH 32
#endif
#ifndef ARITH_STAGE
#define ARITH_STAGE 1
#endif
#ifndef ARITH_PORT
#define ARITH_PORT 1
#endif

typedef bit<ARITH_WIDTH> word_t;
header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header values_t { bit<8> op; bit<64> x; bit<64> y; bit<64> result; }
struct headers_t { ethernet_t ethernet; values_t values; }
struct metadata_t { bit<8> unused; }

#define X ((word_t)hdr.values.x)
#define Y ((word_t)hdr.values.y)

bit<64> calculate(in word_t x, in word_t y, in bit<8> op) {
    bit<64> result = 0;
    if (op == 0) { result = x > y - (word_t)14 ? 64w1 : 64w0; }
    if (op == 1) { result = x - (word_t)20 != y ? 64w1 : 64w0; }
    if (op == 2) { result = (bit<64>)(x - (word_t)3); }
    if (op == 3) { result = (bit<64>)(x - y); }
    if (op == 4) { result = (bit<64>)(x + (word_t)3); }
    if (op == 5) { result = (bit<64>)(x + y); }
    if (op == 6) { result = (bit<64>)(x * y); }
    if (op == 7) { result = (bit<64>)((x - (word_t)3) * y + (word_t)1); }
    if (op == 8) { result = (bit<64>)(x + ~(word_t)0); }
    return result;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        verify(hdr.ethernet.kind == 16w0x88b5, error.NoMatch);
        pkt.extract(hdr.values);
#if ARITH_STAGE == 0
        transition select(hdr.values.op) {
            0: compare_right;
            1: compare_left;
            2: subtract_constant;
            3: subtract_variable;
            4: add_constant;
            5: add_variable;
            6: multiply;
            7: nested;
            8: add_maximum;
            default: reject;
        }
#else
        transition accept;
#endif
    }
#if ARITH_STAGE == 0
    state compare_right {
        verify(X > Y - (word_t)14, error.NoMatch);
        transition accept;
    }
    state compare_left {
        verify(X - (word_t)20 != Y, error.NoMatch);
        transition accept;
    }
    state subtract_constant {
        hdr.values.result = (bit<64>)(X - (word_t)3);
        transition accept;
    }
    state subtract_variable {
        hdr.values.result = (bit<64>)(X - Y);
        transition accept;
    }
    state add_constant {
        hdr.values.result = (bit<64>)(X + (word_t)3);
        transition accept;
    }
    state add_variable {
        hdr.values.result = (bit<64>)(X + Y);
        transition accept;
    }
    state multiply {
        hdr.values.result = (bit<64>)(X * Y);
        transition accept;
    }
    state nested {
        hdr.values.result = (bit<64>)((X - (word_t)3) * Y + (word_t)1);
        transition accept;
    }
    state add_maximum {
        hdr.values.result = (bit<64>)(X + ~(word_t)0);
        transition accept;
    }
#endif
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    action calculate_value() {
        hdr.values.result = calculate((word_t)hdr.values.x, (word_t)hdr.values.y,
                                      hdr.values.op);
    }
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
#if ARITH_STAGE == 1
        hdr.values.result = calculate((word_t)hdr.values.x, (word_t)hdr.values.y,
                                      hdr.values.op);
#elif ARITH_STAGE == 3
        calculate_value();
#endif
        send_to_port((PortId_t)ARITH_PORT);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
#if ARITH_STAGE == 2
        hdr.values.result = calculate((word_t)hdr.values.x, (word_t)hdr.values.y,
                                      hdr.values.op);
#endif
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.values);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;
