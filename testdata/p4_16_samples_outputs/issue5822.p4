#include <core.p4>

parser Parser(packet_in packet);
package SimplePipeline(Parser p);
parser MyParser(packet_in packet) {
    state start {
        transition select() {
            default: s0;
        }
    }
    state s0 {
        transition select() {
            default: accept;
        }
    }
}

SimplePipeline(MyParser()) main;
