#include <core.p4>

parser simple(out bit<2> z);
package m(simple n);
parser p1_0(out bit<2> z1) {
    state start {
        z1 = 2w0;
        transition accept;
    }
}

parser p1_1(out bit<2> z1) {
    state start {
        z1 = 2w1;
        transition accept;
    }
}

parser p1_2(out bit<2> z1) {
    state start {
        z1 = 2w2;
        transition accept;
    }
}

parser p2_0(out bit<2> z2) {
    bit<2> x1_0;
    bit<2> x2_0;
    bit<2> x3_0;
    bit<2> tmp;
    bit<2> tmp_0;
    bit<2> tmp_1;
    bit<2> tmp_2;
    @name("p1a") p1_0() p1a_0;
    @name("p1b") p1_1() p1b_0;
    @name("p1c") p1_2() p1c_0;
    state start {
        p1a_0.apply(x1_0);
        p1b_0.apply(x2_0);
        p1c_0.apply(x3_0);
        tmp = 2w3;
        tmp_0 = tmp | x1_0;
        tmp_1 = tmp_0 | x2_0;
        tmp_2 = tmp_1 | x3_0;
        z2 = tmp_2;
        transition accept;
    }
}

m(p2_0()) main;
