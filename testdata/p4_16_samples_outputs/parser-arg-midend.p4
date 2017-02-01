#include <core.p4>

parser Parser();
package Package(Parser p1, Parser p2);
parser Parser1_0() {
    state start {
        transition accept;
    }
}

Parser1_0() p1;
parser Parser2_0() {
    state start {
        transition accept;
    }
}

Parser2_0() p2;
Package(p1, p2) main;
