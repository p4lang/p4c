#include <core.p4>

extern Local<D> {
    Local();
    D read();
    void write(in D data);
}

control C() {
    @name("C.tmp") bit<32> tmp;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("C.RW") action RW() {
        @name("C.local") Local<bit<32>>() local_0;
        tmp = local_0.read();
        local_0.write(tmp + 32w1);
    }
    @name("C.t") table t_0 {
        actions = {
            RW();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t_0.apply();
    }
}

control CTRL();
package top(CTRL _c);
top(C()) main;
