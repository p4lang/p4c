#include <core.p4>

struct headers {
    bit<32> x;
}

extern void f(headers h);
control c() {
    @hidden action act() {
        f({32w5});
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
    }
}

control C();
package top(C _c);
top(c()) main;

