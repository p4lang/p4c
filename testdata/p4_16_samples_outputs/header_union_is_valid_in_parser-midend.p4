#include <core.p4>

@command_line("--loopsUnroll") header H {
    bit<32> a;
}

header_union HU {
    H h1;
    H h2;
}

parser p(in HU hu, out bool b) {
    @name("tmp") bit<32> tmp_0;
    state start {
        tmp_0 = 32w0;
        transition select(hu.h1.isValid()) {
            true: start_true;
            false: start_join;
        }
    }
    state start_true {
        tmp_0 = tmp_0 + 32w1;
        transition start_join;
    }
    state start_join {
        transition select(hu.h2.isValid()) {
            true: start_true_0;
            false: start_join_0;
        }
    }
    state start_true_0 {
        tmp_0 = tmp_0 + 32w1;
        transition start_join_0;
    }
    state start_join_0 {
        b = tmp_0 == 32w1;
        transition accept;
    }
}

parser proto(in HU hu, out bool b);
package top(proto p);
top(p()) main;
