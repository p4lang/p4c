#include <core.p4>

parser Parser(packet_in packet);
package SimplePipeline(Parser p);
parser MyParser(packet_in packet) {
    state start {
        transition s0;
    }
    state s0 {
        transition accept;
    }
}

SimplePipeline(MyParser()) main;
