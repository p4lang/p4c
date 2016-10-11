#include <core.p4>

control c(inout bit<32> arg) {
    bit<32> tmp;
    @name("a") action a_0() {
    }
    @name("b") action b_0() {
    }
    @name("t") table t_0(in bit<32> x) {
        key = {
            x: exact;
        }
        actions = {
            a_0();
            b_0();
        }
        default_action = a_0();
    }
    apply {
        switch (t_0.apply(arg).action_run) {
            a_0: {
                t_0.apply(arg);
            }
            b_0: {
                tmp = arg + 32w1;
                arg = tmp;
            }
        }

    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
