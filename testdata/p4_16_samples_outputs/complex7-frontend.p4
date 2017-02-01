#include <core.p4>

extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> tmp;
    bool tmp_0;
    bit<32> tmp_1;
    @name("a") action a_0() {
    }
    @name("b") action b_0() {
    }
    @name("t") table t_0(in bit<32> r) {
        key = {
            r: ternary @name("r") ;
        }
        actions = {
            a_0();
            b_0();
        }
        default_action = a_0();
    }
    apply {
        tmp_1 = f(32w2);
        switch (t_0.apply(tmp_1).action_run) {
            a_0: {
                tmp = f(32w2);
                tmp_0 = tmp < 32w2;
                if (tmp_0) 
                    r = 32w1;
                else 
                    r = 32w3;
            }
        }

    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
