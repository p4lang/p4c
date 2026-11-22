#include <core.p4>

parser Parser(packet_in pkt);
package SimpleArch(Parser p);

struct Foo {
    bit<0> f0;
}

parser MyParser(packet_in packet) {
   state start {
       transition select(packet.lookahead<Foo>().f0) {
           0: accept;
           default: reject;
       }
   }
}

SimpleArch(MyParser()) main;
