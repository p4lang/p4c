#include "core.p4"

extern E {
    bit<32> call();
}

control c() {
    action a() {}
    table t(in E e) {
        key = { e.call() : exact; }
        actions = { a; }
        default_action = a;
    }
    E() einst;
    apply {
        t.apply(einst);
    }
}

control none();
package top(none n);

top(c()) main;
