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

parser Parser2_0() {
    Inside() inst_0;
    state start {
        inst_0.apply();
        transition accept;
    }
}

Package(Parser1_0(), Parser2_0()) main;

