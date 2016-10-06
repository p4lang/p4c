#include <core.p4>

extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    action a() {
    }
    action b() {
    }
    table t(in bit<32> r) {
        key = {
            r: ternary;
        }
        actions = {
            a();
            b();
        }
        default_action = a();
    }
    apply {
        switch (t.apply(f(32w2)).action_run) {
            a: {
                if (f(32w2) < 32w2) 
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
