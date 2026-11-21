#include <core.p4>

extern void foo();
extern void bar();
extern bit<8> baz();
action a() {
}
action b() {
}
control c() {
    table t {
        actions = {
            a();
            b();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        switch (baz()) {
            8w1: {
                foo();
            }
            8w4: {
                bar();
            }
        }
        t.apply();
    }
}

control C();
package top(C c);
top(c()) main;
