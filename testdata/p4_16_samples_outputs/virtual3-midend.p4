#include <core.p4>

extern Virtual {
    Virtual();
    void increment();
    @synchronous(increment) abstract bit<16> f(in bit<16> ix);
    bit<16> total();
}

control c(inout bit<16> p) {
    bit<16> local_0;
    bit<16> tmp;
    @name(".NoAction") action NoAction_0() {
    }
    @name("c.cntr") Virtual() cntr_0 = {
        bit<16> f(in bit<16> ix) {
            return ix + local_0;
        }
    };
    @name("c.final_ctr") action final_ctr() {
        tmp = cntr_0.total();
        p = tmp;
    }
    @name("c.add_ctr") action add_ctr() {
        cntr_0.increment();
    }
    @name("c.run_ctr") table run_ctr_0 {
        key = {
            p: exact @name("p") ;
        }
        actions = {
            add_ctr();
            final_ctr();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @hidden action act() {
        local_0 = 16w4;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        run_ctr_0.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

