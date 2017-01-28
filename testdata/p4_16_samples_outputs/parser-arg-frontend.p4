#include <core.p4>

parser Parser();
package Package(Parser p1, Parser p2);
parser Inside() {
    state start {
        transition accept;
    }
}

parser Parser1_0() {
    Inside() inst;
    state start {
        inst.apply();
        transition accept;
    }
}

Parser1_0() p1;
parser Parser2_0() {
    Inside() inst_0;
    state start {
        inst_0.apply();
        transition accept;
    }
}

Parser2_0() p2;
Package(p1, p2) main;
