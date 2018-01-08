#include <core.p4>

parser Parser();
package Package(Parser p1, Parser p2);
parser Parser1_0() {
    state start {
        transition Inside_start;
    }
    state Inside_start {
        transition start_0;
    }
    state start_0 {
        transition accept;
    }
}

parser Parser2_0() {
    state start {
        transition Inside_start_0;
    }
    state Inside_start_0 {
        transition start_1;
    }
    state start_1 {
        transition accept;
    }
}

Package(Parser1_0(), Parser2_0()) main;

