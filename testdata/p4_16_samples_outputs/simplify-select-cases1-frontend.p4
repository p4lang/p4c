#include <core.p4>

parser p(packet_in packet) {
    @name("p.tmp") bit<8> tmp;
    @name("p.tmp_0") bit<8> tmp_0;
    @name("p.tmp_1") bit<16> tmp_1;
    @name("p.tmp_2") bit<16> tmp_2;
    @name("p.tmp_3") bit<16> tmp_3;
    @name("p.tmp_4") bit<16> tmp_4;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        tmp = tmp_0;
        transition select(tmp) {
            8w1: state_1;
            8w2: state_2;
        }
    }
    state state_1 {
        tmp_2 = packet.lookahead<bit<16>>();
        tmp_1 = tmp_2;
        transition select(tmp_1) {
            16w0x21: state_4;
            16w0x21: state_3;
            16w0x22: accept;
            default: state_3;
        }
    }
    state state_2 {
        tmp_4 = packet.lookahead<bit<16>>();
        tmp_3 = tmp_4;
        transition select(tmp_3) {
            16w0x21: state_3;
            16w0x21: state_4;
            16w0x22: accept;
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
