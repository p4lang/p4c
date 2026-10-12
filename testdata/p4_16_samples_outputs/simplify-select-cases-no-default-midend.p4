#include <core.p4>


parser p(packet_in packet) {
    @name("p.tmp_0") bit<8> tmp_0;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        transition select(tmp_0) {
            8w0x11: accept;
            default: noMatch;
        }
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

parser simple(packet_in packet);
package top(simple prs);
top(p()) main;
