#include <core.p4>

parser p(packet_in packet) {
    @name("p.tmp_0") bit<8> tmp_0;
    @name("p.tmp_2") bit<16> tmp_2;
    @name("p.tmp_4") bit<16> tmp_4;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        transition select(tmp_0) {
            8w1: state_1;
            8w2: state_2;
            default: noMatch;
        }
    }
    state state_1 {
        tmp_2 = packet.lookahead<bit<16>>();
        transition select(tmp_2) {
            16w0x21: state_4;
            16w0x22: accept;
            default: state_3;
        }
    }
    state state_2 {
        tmp_4 = packet.lookahead<bit<16>>();
        transition select(tmp_4) {
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
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser simple(packet_in packet);
package top(simple e);
top(p()) main;
