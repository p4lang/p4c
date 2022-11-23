#include <core.p4>

struct S<T> {
    T t;
}

struct S_0 {
    bit<32> t;
}

extern E {
    E(list<S_0> data);
    void run();
}

control c() {
    @name("c.e") E((list<S_0>){(S_0){t = 32w10},(S_0){t = 32w5}}) e_0;
    @hidden action list9l15() {
        e_0.run();
    }
    @hidden table tbl_list9l15 {
        actions = {
            list9l15();
        }
        const default_action = list9l15();
    }
    apply {
        tbl_list9l15.apply();
    }
}

control C();
package top(C _c);
top(c()) main;
