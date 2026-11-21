#include <core.p4>


parser p(packet_in packet) {
    @name("p.tmp") bit<8> tmp;
    @name("p.tmp_0") bit<8> tmp_0;
    state start {
        tmp_0 = packet.lookahead<bit<8>>();
        tmp = tmp_0;
        transition select(tmp) {
            8w0x11: accept;
            8w0x11: reject;
        }
    }
}

parser simple(packet_in packet);
package top(simple prs);
top(p()) main;
