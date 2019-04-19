#include <core.p4>

parser Parser();
package Package(Parser p1, Parser p2);
parser Parser1_0() {
    state start {
        transition accept;
    }
}

parser Parser2_0() {
    state start {
        transition accept;
    }
}

Package(Parser1_0(), Parser2_0()) main;

