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
            a;
            b;
        }
    }
    apply {
        switch (baz()) {
            (1 + 2 == 3 ? 1 : 2): {
                foo();
            }
            (3 + 4 == 0 ? 3 : 4): {
                bar();
            }
        }
        t.apply();
    }
}

control C();
package top(C c);
top(c()) main;
