#include <core.p4>

parser Parser(packet_in packet);
package SimplePipeline(Parser p);
struct EmptyKey {
}

parser MyParser(packet_in packet) {
    value_set<EmptyKey>(8) pvs;
    state start {
        transition select() {
            pvs: s0;
            default: accept;
        }
    }
    state s0 {
        EmptyKey empty;
        transition s1;
    }
    state s1 {
        tuple<> empty;
        transition reject;
    }
}

SimplePipeline(MyParser()) main;
