#include <core.p4>

extern Virtual {
    Virtual();
    void increment();
    @synchronous(increment) abstract bit<16> f(in bit<16> ix);
    bit<16> total();
}

control c(inout bit<16> p) {
    @name("c.local") bit<16> local_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("c.cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + local_0;
        }
    };
    @name("c.final_ctr") action final_ctr() {
        p = cntr_0.total();
    }
    @name("c.add_ctr") action add_ctr() {
        cntr_0.increment();
    }
    @name("c.run_ctr") table run_ctr_0 {
        key = {
            p: exact @name("p");
        }
        actions = {
            add_ctr();
            final_ctr();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        local_0 = 16w4;
        run_ctr_0.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;
