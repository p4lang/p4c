#include <core.p4>

parser p(packet_in packet) {
    state start {
        transition select(packet.lookahead<bit<8>>()) {
            8w1: state_1;
            8w2: state_2;
        }
    }
    state state_1 {
        transition select(packet.lookahead<bit<16>>(), (packet.lookahead<bit<32>>())[15:0]) {
            (16w0x21, 16w0x22): state_4;
            (16w0x21, 16w0x22): state_3;
            (16w0x23, 16w0x24): accept;
            default: state_3;
        }
    }
    state state_2 {
        transition select(packet.lookahead<bit<16>>(), (packet.lookahead<bit<32>>())[15:0]) {
            (16w0x21, 16w0x22): state_3;
            (16w0x21, 16w0x22): state_4;
            (16w0x23, 16w0x24): accept;
            default: state_3;
        }
    }
    state state_3 {
        transition accept;
    }
    state state_4 {
        transition accept;
    }
}

parser simple(packet_in packet);
package top(simple e);
top(p()) main;
