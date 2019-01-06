#include <core.p4>

extern Virtual {
    Virtual();
    void increment();
    @synchronous(increment) abstract bit<16> f(in bit<16> ix);
    bit<16> total();
}

control c(inout bit<16> p) {
    bit<16> local;
    Virtual() cntr = {
        bit<16> f(in bit<16> ix) {
            return ix + local;
        }
    };
    action final_ctr() {
        p = cntr.total();
    }
    action add_ctr() {
        cntr.increment();
    }
    table run_ctr {
        key = {
            p: exact @name("p") ;
        }
        actions = {
            add_ctr();
            final_ctr();
            @defaultonly NoAction();
        }
        default_action = NoAction();
    }
    apply {
        local = 16w4;
        run_ctr.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);
top(c()) main;

