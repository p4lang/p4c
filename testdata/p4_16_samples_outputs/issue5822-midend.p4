#include <core.p4>

parser Parser(packet_in packet);
package SimplePipeline(Parser p);
struct EmptyKey {
}

parser MyParser(packet_in packet) {
    @name("MyParser.pvs") value_set<EmptyKey>(8) pvs_0;
    state start {
        transition select() {
            pvs_0: s0;
            default: accept;
        }
    }
    state s0 {
        transition reject;
    }
}

SimplePipeline(MyParser()) main;
