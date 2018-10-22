#include <core.p4>

parser par(out bool b) {
    bit<32> x_0;
    bit<32> y_0;
    bit<32> x_1;
    state start {
        y_0 = 32w0;
        transition adder_0_start;
    }
    state adder_0_start {
        x_1 = y_0 + 32w6;
        transition start_0;
    }
    state start_0 {
        x_0 = x_1;
        b = x_0 == 32w0;
        transition accept;
    }
}

control c(out bool b) {
    bit<16> xv_0;
    bit<16> x_2;
    bool b_0;
    bit<16> x_3;
    bool b_1;
    @name("c.a") action a(in bit<16> bi, out bit<16> mb) {
        mb = -bi;
    }
    @name("c.a") action a_2(in bit<16> bi_1, out bit<16> mb_1) {
        mb_1 = -bi_1;
    }
    apply {
        a(bi = 16w3, mb = xv_0);
        a_2(mb = xv_0, bi = 16w0);
        x_2 = xv_0;
        b_0 = x_2 == 16w0;
        b = b_0;
        xv_0 = x_2;
        x_3 = xv_0;
        b_1 = x_3 == 16w1;
        xv_0 = x_3;
        b = b_1;
        xv_0 = 16w1;
        x_2 = xv_0;
        b_0 = x_2 == 16w0;
        xv_0 = x_2;
        b = b_0;
        x_3 = xv_0;
        b_1 = x_3 == 16w1;
        b = b_1;
        xv_0 = x_3;
    }
}

control ce(out bool b);
parser pe(out bool b);
package top(pe _p, ce _e, @optional ce _e1);
top(_e = c(), _p = par()) main;

