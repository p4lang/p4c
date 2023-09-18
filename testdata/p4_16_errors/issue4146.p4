#include <core.p4>

parser Parser(packet_in pkt);
package SimpleArch(Parser p);

parser MyParser(packet_in packet) {
   state start {
       packet.lookahead<void>();
       transition accept;
   }
}

SimpleArch(MyParser()) main;
