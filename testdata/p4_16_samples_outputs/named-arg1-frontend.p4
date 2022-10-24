#include <core.p4>

parser par(out bool b) {
    @name("par.x") bit<32> x_0;
    state start {
        transition adder_0_start;
    }
    state adder_0_start {
        x_0 = 32w0 + 32w6;
        transition start_0;
    }
    state start_0 {
        b = x_0 == 32w0;
        transition accept;
    }
}

control c(out bool b) {
    @name("c.xv") bit<16> xv_0;
    @name("c.x_1") bit<16> x_1;
    @name("c.x_2") bit<16> x_2;
    @name("c.b_1") bool b_1;
    @name("c.a") action a() {
    }
    @name("c.a") action a_1() {
    }
    apply {
        a();
        a_1();
        xv_0 = 16w1;
        x_1 = xv_0;
        xv_0 = x_1;
        x_2 = xv_0;
        b_1 = x_2 == 16w1;
        b = b_1;
    }
}

control ce(out bool b);
parser pe(out bool b);
package top(pe _p, ce _e, @optional ce _e1);
top(_e = c(), _p = par()) main;
