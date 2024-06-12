#include <core.p4>

extern Local<D> {
    Local();
    D read();
    void write(in D data);
}

control C() {
    action RW() {
        Local<bit<32>>() local;
        local.write(local.read() + 32w1);
    }
    table t {
        actions = {
            RW();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        t.apply();
    }
}

control CTRL();
package top(CTRL _c);
top(C()) main;
