#include <core.p4>

parser Parser(packet_in pkt);
package SimpleArch(Parser p);
struct Foo {
    bit<0> f0;
}

parser MyParser(packet_in packet) {
    @name("MyParser.tmp_0") Foo tmp_0;
    state start {
        packet.lookahead<bit<0>>();
        transition select(tmp_0.f0) {
            0: accept;
            default: reject;
        }
    }
}

SimpleArch(MyParser()) main;
