#include <core.p4>

extern Local<D> {
    Local();
    D read();
    void write(in D data);
}

control C() {
    action RW() {
        Local<bit<32>>() local;
        local.write(local.read() + 1);
    }
    table t {
        actions = {
            RW;
        }
    }
    apply {
        t.apply();
    }
}

control CTRL();
package top(CTRL _c);
top(C()) main;
