#include <core.p4>

parser Parser(packet_in packet);
package SimplePipeline(Parser p);
parser MyParser(packet_in packet) {
    state start {
        transition accept;
    }
}

SimplePipeline(MyParser()) main;
