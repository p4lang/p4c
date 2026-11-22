#include <core.p4>

parser Parser(packet_in pkt);
package SimpleArch(Parser p);
struct Foo {
    bit<0> f0;
}

parser MyParser(packet_in packet) {
    @name("MyParser.tmp") bit<0> tmp;
    @name("MyParser.tmp_0") Foo tmp_0;
    state start {
        tmp_0 = packet.lookahead<Foo>();
        tmp = tmp_0.f0;
        transition select(tmp) {
            0: accept;
            default: reject;
        }
    }
}

SimpleArch(MyParser()) main;
