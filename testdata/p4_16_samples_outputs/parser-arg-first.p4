#include <core.p4>

parser Parser();
package Package(Parser p1, Parser p2);
parser Parser1()(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

parser Parser2()(Parser p) {
    state start {
        p.apply();
        transition accept;
    }
}

parser Inside() {
    state start {
        transition accept;
    }
}

Package(Parser1(Inside()), Parser2(Inside())) main;

